#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

#include<QRegExp>
#include<QMessageBox>
//提交登录信息
void LoginDialog::on_pb_commit_clicked()
{
    QString strTel = ui->le_tel->text();
    QString strPassword = ui->le_password->text();
    //校验
    //非空校验 清掉空格之后，不能是空字符串
    QString tmpTel = strTel,tmpPass = strPassword;
    if(tmpTel.remove(" ").isEmpty() || tmpPass.remove(" ").isEmpty()){
        QMessageBox::about(this,"提示","手机号密码不能为空格");
        return;
    }
    //1.校验手机号 正则表达式
    QRegExp reg("^1[3-8][0-9]\{6,9\}$");
    bool res = reg.exactMatch(strTel);
    if(!res){
        QMessageBox::about(this,"提示","手机号格式错误，8-11位手机号");
        return;
    }
    //2.校验密码 长度不能超过20
    if(strPassword.length() > 20){
        QMessageBox::about(this,"提示","密码过长，长度不超过20");
        return;
    }
    Q_EMIT SIG_loginCommit(strTel,strPassword);
}

//清空注册信息
void LoginDialog::on_pb_clear_register_clicked()
{
    ui->le_tel_register->setText("");
    ui->le_password_register->setText("");
    ui->le_confirm_register->setText("");
    ui->le_name_register->setText("");
}

//提交注册信息
void LoginDialog::on_pb_commit_register_clicked()
{
    QString strTel = ui->le_tel_register->text();
    QString strPassword = ui->le_password_register->text();
    QString strConfirm = ui->le_confirm_register->text();
    QString strName = ui->le_name_register->text();
    //校验
    //非空校验 清掉空格之后，不能是空字符串
    QString tmpTel = strTel,tmpPass = strPassword,tmpName = strName;
    if(tmpTel.remove(" ").isEmpty() || tmpPass.remove(" ").isEmpty() || strName.remove(" ").isEmpty()){
        QMessageBox::about(this,"提示","手机号密码昵称不能为空格");
        return;
    }
    //1.校验手机号 正则表达式
    QRegExp reg("^1[3-8][0-9]\{6,9\}$");
    bool res = reg.exactMatch(strTel);
    if(!res){
        QMessageBox::about(this,"提示","手机号格式错误，8-11位手机号");
        return;
    }
    //2.校验密码 长度不能超过20
    if(strPassword.length() > 20){
        QMessageBox::about(this,"提示","密码过长，长度不超过20");
        return;
    }
    //3.两次输入是否一致
    if(strPassword != strConfirm){
        QMessageBox::about(this,"提示","两次密码不一致");
        return;
    }
    //name长度不超过10
    if(strName.length() > 10){
        QMessageBox::about(this,"提示","昵称长度不超过10");
        return;
    }

    Q_EMIT SIG_registerCommit(strTel,strPassword,strName);
}

//清空登录信息
void LoginDialog::on_pb_clear_clicked()
{
    ui->le_tel->setText("");
    ui->le_password->setText("");
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    Q_EMIT SIG_close();
}

