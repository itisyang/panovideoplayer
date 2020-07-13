#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ffmepgdemo.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QImage>
#include <QTimer>

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"

}

class FFmepgDemo : public QMainWindow
{
    Q_OBJECT

public:
    FFmepgDemo(QWidget *parent = Q_NULLPTR);
    int ChangeViewPoint();

    void ShowTimerOut();

    void OpenPano(QString image_path);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
private slots:
    void on_pushbutton_open_clicked();
private:
    Ui::FFmepgDemoClass ui;

    QPoint pos_mouse_ctrl_press_;

    float yaw_;   
    float pitch_; 
    float roll_; 
    float field_of_view_; //h_fov v_fov


    QImage image_;

    QTimer timer;



};
