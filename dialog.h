#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QtNetwork>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_sender_clicked();
    void getdata(int sid);  //接收数据，分类处理
    void datahandler(int sid, QByteArray data);  //处理数据发送
    void sendfiles(int sid);  //文件信息发送
    void sendack(int sid, int fid, int num);  //ack回复
    void ackdeal(int sid, QByteArray data);  //收到ack
    void resend(int sid, int num);  //超时重传
    void reorder(int sid, QByteArray data);  //乱序重传
    void timesender(int sid);

private:
    Ui::Dialog *ui;
    QUdpSocket *seversock[5];  //为每个用户新分配端口sock1-4,0为共享（收广播）
    QString root;  //根目录
    QString filename[5];  //文件名
    QTimer *tf;  //文件列表定时器
    QTimer *sendtm[5];
    QTimer *timer[3][24];  //定时器
    QHostAddress ip;
    quint16 pt;  //允许多用户
    quint16 idpt[5];
    quint16 curclient;
    int curnum[5];
    int csid;
    int sidfl[5];  //当前客户端在下载的文件
    QByteArray buff;
};
#endif // DIALOG_H
