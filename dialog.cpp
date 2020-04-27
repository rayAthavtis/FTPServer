#include "dialog.h"
#include "ui_dialog.h"

#include <QFileDialog>

/*
模仿传输层TCP的可靠性传输:
1、添加seq/ack机制，确保数据成功发送给对方；
2、添加发送和接收缓冲区；
3、添加超时重传机制。
送端发送数据时，发送定长文件数据块，带块号。
数据到达接收端后接收端放入缓存，并发送一个ack=x的包，表示已经收到数据。
发送端收到了ack包后，删除定时器。
定时任务检查是否需要重传数据。

实验要求：
1.下层使用UDP协议（即使用数据包套接字完成本次程序）；
2.完成客户端和服务器端程序；
3.实现可靠的文件传输：能可靠下载文件，能同时下载文件。
*/

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //初始化
    curclient = 0;
    for(int i = 0; i < 5; i++)
    {
        seversock[i] = new QUdpSocket(this);
        sidfl[i] = -1;
        idpt[i] = 0;
        seversock[i]->bind(11005 + curclient,QUdpSocket::ShareAddress);
        //Lambda匿名函数，传参
        connect(seversock[i],&QUdpSocket::readyRead,this,[=]{getdata(i);});
        curclient += 1;
    }
    tf = new QTimer;
    tf->setInterval(1000);
    tf->setSingleShot(true);
    connect(tf,&QTimer::timeout,this,[=]{sendfiles(csid);});
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 24; j++)
        {
            timer[i][j] = new QTimer;
            timer[i][j]->setInterval(1000);  //设置时限
            timer[i][j]->setSingleShot(true);
            connect(timer[i][j],&QTimer::timeout,this,[=]{resend(i,j);});
        }
    }
    for(int i = 0; i < 5; i++)
    {
        sendtm[i] = new QTimer;
        sendtm[i]->setInterval(1000);  //设置时限
        curnum[i] = 0;
        connect(sendtm[i],&QTimer::timeout,this,[=]{
            sendtm[i]->blockSignals(true);  //阻塞
            timesender(i);
            sendtm[i]->blockSignals(false);});
    }
    csid = 0;
    root = "/Users/renlei/Qt-workspace/shareplace";
    ui->logshow->append("****** 服务器就绪 ******");
}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::on_sender_clicked()  //初期测试用的，弃
{
    QByteArray data = "hello client. ";
    //四个参数分别是数据报的内容，数据报的大小，主机地址和端口号
    seversock[0]->writeDatagram(data.data(),data.size(),QHostAddress::Broadcast,pt);
    ui->logshow->append("发送成功。");
}

void Dialog::sendfiles(int sid)
{
    ui->logshow->append("收到文件列表信息请求。");
    sendack(sid,-1,0);  //回复收到文件列表请求
    QDir dir(root);
    if(!dir.exists())  //文件夹不存在
    {
        ui->logshow->append("文件夹不存在。");
        return ;
    }
    dir.setFilter(QDir::Files);  //除了文件，其他的过滤掉
    QFileInfoList filelist = dir.entryInfoList();  //获取文件信息列表
    int curfile = 0;
    for(int i = 0; i < filelist.size(); i++)
    {
         QFileInfo fileinfo = filelist.at(i);
         QString fn = fileinfo.fileName();
         filename[curfile] = fn;  //文件分配id
         curfile += 1;
    }
    for(int i = 0; i < curfile; i++)
    {
        QFileInfo fileinfo = filelist.at(i);
        QString packet = "1\r\n" + filename[i] + "\r\n" + QString::number(fileinfo.size());
        QByteArray sf = packet.toLocal8Bit();
        seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
    }
    ui->logshow->append("成功发送文件列表信息。");
    tf->start();  //开始计时
}

void Dialog::sendack(int sid, int fid, int num)  //op+fid+num
{
    QString tmp = "5\r\n" + QString::number(fid) + "\r\n" + QString::number(num);
    QByteArray sp = tmp.toLocal8Bit();
    seversock[sid]->writeDatagram(sp.data(),sp.size(),ip,idpt[sid]);
}

void Dialog::datahandler(int sid, QByteArray data)  //op + filename
{
    ui->logshow->append("收到文件下载请求。");
    sendack(sid,-1,1);
    QString fn = data.right(data.size()-3);
    ui->logshow->append("客服端请求下载文件：" + fn);
    int cf = -1;  //文件id
    for(int i = 0; i < 5; i++)  //查询文件id
    {
        if(filename[i] == fn)
        {
            cf = i;
            break;
        }
    }
    sidfl[sid] = cf;  //当前sid下载的文件id为cf
    //int curnum = 0;  //块号初始化
    if(fn.right(3) == "txt")  //文件下载，陆续发送
        sendtm[sid]->start();
    else if(fn.right(3) == "jpg" || fn.right(3) == "png")
        sendtm[sid]->start();
}

void Dialog::timesender(int sid)
{
    int fid = sidfl[sid];
    QString asfn = root + "/" + filename[fid];
    QFile file(asfn);
    if(!file.exists())
    {
        ui->logshow->append("文件" + filename[fid] + "不存在！");
        file.close();
        return ;
    }
    if(!file.open(QIODevice::ReadOnly))
    {
        ui->logshow->append("文件" + filename[fid] + "打开失败！");
        return ;
    }
    if(filename[fid].right(3) == "txt")
    {
        QTextStream in(&file);  //文本读取
        in.read(512*curnum[sid]);
        if(!in.atEnd())
        {
            QString buf = in.read(512);  //每次读取大小为512
            if(buf.size() > 0)  //成功读取
            {
                QString tmp = buf;
                QString packet = "3\r\n" + QString::number(fid) + "\r\n" +
                        QString::number(curnum[sid])+ "\r\n" + QString::number(buf.size())
                        + "\r\n" + tmp;
                QByteArray sf = packet.toLocal8Bit();
                seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
                timer[sid][curnum[sid]]->start();
                ui->logshow->append("发送文件：" + filename[fid] + "-" +
                        QString::number(curnum[sid]) + "-" + QString::number(buf.size()));
            }
            curnum[sid] += 1;
        }
        else
        {
            curnum[sid] = 0;
            sendtm[sid]->stop();
        }
        file.close();
    }
    else if(filename[fid].right(3) == "jpg" || filename[fid].right(3) == "png")
    {
        QDataStream out(&file);
        int ln = 2048*curnum[sid];
        char *trash = new char[ln];
        char *buf = new char[2048];
        out.readRawData(trash, ln);
        if(!out.atEnd())
        {
            int l = out.readRawData(buf, 2048);  //每次读取大小为2048
            QString tmp = buf;
            QString packet = "3\r\n" + QString::number(fid) + "\r\n" + QString::number(curnum[sid])
                    + "\r\n" + QString::number(l) + "\r\n" + tmp;
            QByteArray sf = packet.toLocal8Bit();
            seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
            timer[sid][curnum[sid]]->start();
            ui->logshow->append("发送文件：" + filename[fid] + "-" + QString::number(curnum[sid])
                                + "-" + QString::number(l));
            curnum[sid] += 1;
        }
        else
        {
            curnum[sid] = 0;
            sendtm[sid]->stop();
        }
        file.close();
    }
}

void Dialog::ackdeal(int sid, QByteArray data)  //op+fn+num
{
    QString tmp = data;
    QStringList tp = tmp.split("\r\n");
    int fid = tp.at(1).toInt();
    int num = tp.at(2).toInt();
    if(fid == -1 && num == 2)  //文件列表信息成功被接收
    {
        tf->stop();  //关闭计时
        ui->logshow->append("客户端成功接收文件列表信息。");
    }
    else  //文件下载数据块成功到达客户端
    {
        timer[sid][num]->stop();  //关闭计时
        ui->logshow->append("客户端成功接收：" + filename[fid] + "-" + tp.at(2));
    }
}

void Dialog::resend(int sid, int num)  //超时重传
{
    QString fn = filename[sidfl[sid]];
    int cnum = num;
    QString asfn = root + "/" + fn;
    int cf = -1;  //文件id
    ui->logshow->append(fn + "-" + QString::number(cnum) +"超时重传。");
    QFile file(asfn);
    if(!file.exists())
    {
        ui->logshow->append("文件" + fn + "不存在！");
        file.close();
        return ;
    }
    if(!file.open(QIODevice::ReadWrite))
    {
        ui->logshow->append("文件" + fn + "打开失败！");
        return ;
    }
    for(int i = 0; i < 5; i++)  //查询文件id
    {
        if(filename[i] == fn)
        {
            cf = i;
            break;
        }
    }
    if(fn.right(3) == "txt")  //文件下载
    {
        QTextStream in(&file);  //文本读取
        in.read(512*cnum);  //定位
        if(!in.atEnd())  //哪块超时传哪块
        {
            QString buf = in.read(512);  //每次读取大小为512
            if(buf.size() > 0)  //成功读取
            {
                QString tmp = buf;
                QString packet = "3\r\n" + QString::number(cf) + "\r\n" + QString::number(cnum)
                        + "\r\n" + QString::number(buf.size()) + "\r\n" + tmp;
                QByteArray sf = packet.toLocal8Bit();
                seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
                timer[sid][cnum]->start();
                ui->logshow->append("发送文件：" + filename[cf] + "-" + QString::number(cnum)
                                    + "-" + QString::number(buf.size()));
            }
            cnum += 1;
        }
        file.close();
    }
    else if(fn.right(3) == "jpg" || fn.right(3) == "png")
    {
        QDataStream out(&file);
        int ln = 2048*cnum;
        char *trash = new char[ln];
        char *buf = new char[2048];
        out.readRawData(trash, ln);
        if(!out.atEnd())
        {
            int l = out.readRawData(buf, 2048);  //每次读取大小为2048
            QString tmp = buf;
            QString packet = "3\r\n" + QString::number(cf) + "\r\n" + QString::number(cnum)
                    + "\r\n" + QString::number(l) + "\r\n" + tmp;
            QByteArray sf = packet.toLocal8Bit();
            seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
            timer[sid][cnum]->start();
            ui->logshow->append("发送文件：" + filename[cf] + "-" + QString::number(cnum)
                                + "-" + QString::number(l));
            cnum += 1;
        }
        file.close();
    }
}

void Dialog::reorder(int sid, QByteArray data)  //op+fn+num
{
    ui->logshow->append("收到重传请求。");
    sendack(sid,-1,2);
    QString tmp = data;
    QStringList tp = tmp.split("\r\n");
    QString fn = filename[sidfl[sid]];
    int cnum = tp.at(2).toInt();
    QString asfn = root + "/" + fn;
    int cf = -1;  //文件id
    ui->logshow->append("客户端请求重传：" + fn + "-" + QString::number(cnum));
    QFile file(asfn);
    if(!file.exists())
    {
        ui->logshow->append("文件" + fn + "不存在！");
        file.close();
        return ;
    }
    if(!file.open(QIODevice::ReadWrite))
    {
        ui->logshow->append("文件" + fn + "打开失败！");
        return ;
    }
    for(int i = 0; i < 5; i++)  //查询文件id
    {
        if(filename[i] == fn)
        {
            cf = i;
            break;
        }
    }
    if(fn.right(3) == "txt")  //文件下载
    {
        QTextStream in(&file);  //文本读取
        in.read(512*cnum);  //之后的数据都重发一遍
        while(!in.atEnd())
        {
            QString buf = in.read(512);  //每次读取大小为512
            if(buf.size() > 0)  //成功读取
            {
                QString tmp = buf;
                QString packet = "3\r\n" + QString::number(cf) + "\r\n" + QString::number(cnum)
                        + "\r\n" + QString::number(buf.size()) + "\r\n" + tmp;
                QByteArray sf = packet.toLocal8Bit();
                seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
                timer[sid][cnum]->stop();
                timer[sid][cnum]->start();
                ui->logshow->append("发送文件：" + filename[cf] + "-" + QString::number(cnum)
                                    + "-" + QString::number(buf.size()));
            }
            cnum += 1;
        }
        file.close();
    }
    else if(fn.right(3) == "jpg" || fn.right(3) == "png")
    {
        QDataStream out(&file);
        int ln = 2048*cnum;
        char *trash = new char[ln];
        char *buf = new char[2048];
        out.readRawData(trash, ln);
        while(!out.atEnd())
        {
            int l = out.readRawData(buf, 2048);  //每次读取大小为2048
            QString tmp = buf;
            QString packet = "3\r\n" + QString::number(cf) + "\r\n" + QString::number(cnum)
                    + "\r\n" + QString::number(l) + "\r\n" + tmp;
            QByteArray sf = packet.toLocal8Bit();
            seversock[sid]->writeDatagram(sf.data(),sf.size(),ip,idpt[sid]);
            timer[sid][cnum]->start();
            ui->logshow->append("发送文件：" + filename[cf] + "-" + QString::number(cnum)
                                + "-" + QString::number(l));
            cnum += 1;
        }
        file.close();
    }
}

void Dialog::getdata(int sid)
{
    while(seversock[sid]->hasPendingDatagrams())  //拥有等待的数据
    {
        QByteArray data;  //用于存放接收的数据
        qint64 size=seversock[sid]->pendingDatagramSize();
        data.resize(size);
        seversock[sid]->readDatagram(data.data(),size,&ip,&pt);
        //ui->logshow->append(data);
        int fl = 0;
        for(int i = 0; i < 5; i++)
        {
            if(idpt[i] == pt)
            {
                fl = 1;
                break;
            }
        }
        if(fl == 0)  //新客户端，需分配端口
        {
            csid +=1 ;
            idpt[csid] = pt;
        }
        if(data.at(0) == '0')  //请求共享文件名
            sendfiles(csid);
        else if(data.at(0) == '2')  //收到文件下载请求
            datahandler(sid, data);
        else if(data.at(0) == '4')  //收到ack回复
            ackdeal(sid, data);
        else if(data.at(0) == '6')  //请求重传
            reorder(sid, data);
    }
}
