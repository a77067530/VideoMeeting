#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include<QCloseEvent>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_loginCommit(QString tel,QString password);

    void SIG_registerCommit(QString tel,QString password,QString name);

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    void closeEvent(QCloseEvent * event);
signals:
    void SIG_close();
private slots:
    void on_pb_commit_clicked();

    void on_pb_clear_register_clicked();

    void on_pb_commit_register_clicked();

    void on_pb_clear_clicked();


private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
