#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_vlcdemo.h"

#include <QTimer>
#include <QMouseEvent>

#include <stdio.h>
#include <stdlib.h>


#include <vlc/vlc.h>


class VLCDemo : public QMainWindow
{
    Q_OBJECT

public:
    VLCDemo(QWidget *parent = Q_NULLPTR);
    ~VLCDemo();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
private:
    Ui::VLCDemoClass ui;


    libvlc_instance_t * inst;
    libvlc_media_player_t *mp;
    libvlc_media_list_player_t *mlp;
    libvlc_media_list_t *ml;

    libvlc_video_viewpoint_t *view;

    QTimer *timer;

    QPoint pos_mouse_ctrl_press_;

};
