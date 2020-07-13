#include "vlcdemo.h"

#include <chrono>
#include <thread>
#include <iostream>

#include <QDebug>

#include <windows.h>

#pragma execution_character_set("utf-8")


#define qtu( i ) ((i).toUtf8().constData())

VLCDemo::VLCDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //ui.widget_play->setMouseTracking(true);

    /* Load the VLC engine */
    inst = libvlc_new(0, NULL);

    /* Create a new item */
    //m = libvlc_media_new_location (inst, "http://mycool.movie.com/test.mov");
    //m = libvlc_media_new_path (inst, "/path/to/test.mov");


    if (!inst)
    {
        const char* msg = libvlc_errmsg();
        std::cout << msg << endl;
    }


    ml = libvlc_media_list_new(this->inst);

    QString video_file = "F:/Downloads/8k video/4K 2D.mp4";
    libvlc_media_t *m = libvlc_media_new_path(inst, "4K 2D.mp4");

    libvlc_media_list_add_media(ml, m);

    /* No need to keep the media now */
    libvlc_media_release(m);

    mlp = libvlc_media_list_player_new(inst);
    /* Create a media player playing environement */
    //mp = libvlc_media_player_new_from_media(m);
    mp = libvlc_media_player_new(this->inst);
    libvlc_media_list_player_set_media_list(mlp, ml);
    libvlc_media_list_player_set_media_player(mlp, mp);
    libvlc_media_list_player_set_playback_mode(mlp, libvlc_playback_mode_loop);



    /* This is a non working code that show how to hooks into a window,
          * if we have a window around */
          //libvlc_media_player_set_hwnd (mp, hwnd);

    libvlc_media_player_set_hwnd(mp, (HWND)ui.widget_play->winId());

    /* play the media_player */
    //libvlc_media_player_play(mp);

    libvlc_media_list_player_play(mlp);

    int width = 0, height = 0;
    libvlc_video_get_size(mp, 0, (unsigned *)&width, (unsigned*)&height);

    printf("Resolution: %d x %d\n", width, height);

    std::cout << width << height << std::endl;


    view = libvlc_video_new_viewpoint();
    view->f_yaw = 0;
    view->f_pitch = 0;
    view->f_roll = 0;
    view->f_field_of_view = 180;
    libvlc_video_update_viewpoint(mp, view, true);


    //libvlc无法响应鼠标消息  https://blog.jianchihu.net/player-based-on-libvlc.html
    // https://forum.videolan.org/viewtopic.php?f=32&t=92568#
    EnableWindow((HWND)ui.widget_play->winId(), false);

}

VLCDemo::~VLCDemo()
{
    /* Stop playing */
    libvlc_media_player_stop(mp);

    /* Free the media_player */
    libvlc_media_player_release(mp);

    libvlc_release(inst);
}

void VLCDemo::mousePressEvent(QMouseEvent *event)
{
    //qDebug() << "VLCDemo::mousePressEvent";

    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.widget_play->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.widget_play->mapToGlobal(QPoint(ui.widget_play->width(), ui.widget_play->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);

    if (rect_preview.contains(pos_event))
    {
        pos_mouse_ctrl_press_ = pos_event;
    }
}

void VLCDemo::mouseReleaseEvent(QMouseEvent *event)
{

}

void VLCDemo::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "VLCDemo::mouseMoveEvent";

    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.widget_play->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.widget_play->mapToGlobal(QPoint(ui.widget_play->width(), ui.widget_play->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);


    if (rect_preview.contains(pos_event))
    {
        int x_move = pos_event.x() - pos_mouse_ctrl_press_.x();
        int y_move = pos_event.y() - pos_mouse_ctrl_press_.y();

        int move_abs = 10;
        if (move_abs < x_move || move_abs * (-1) > x_move)
        {
            int change_index = x_move / move_abs;
            view->f_yaw -= change_index;
            if (view->f_yaw > 180)
            {
                view->f_yaw = -180;
            }
            if (view->f_yaw < -180)
            {
                view->f_yaw = 180;
            }

            qDebug() << "yaw" << view->f_yaw << " " << "pitch" << view->f_pitch << " " << "roll" << view->f_roll << " " << "field" << view->f_field_of_view;

            libvlc_video_update_viewpoint(mp, view, true);
        }

        if (move_abs < y_move || move_abs * (-1) > y_move)
        {
            int change_index = y_move / move_abs;
            view->f_pitch -= change_index;
            if (view->f_pitch > 180)
            {
                view->f_pitch = -180;
            }
            if (view->f_pitch < -180)
            {
                view->f_pitch = 180;
            }
            float f_yaw;           /**< view point yaw in degrees  ]-180;180] */
            float f_pitch;         /**< view point pitch in degrees  ]-90;90] */
            float f_roll;          /**< view point roll in degrees ]-180;180] */
            float f_field_of_view; /**< field of view in degrees ]0;180[ (default 80.)*/
            qDebug() << "yaw" << view->f_yaw << " " << "pitch" << view->f_pitch << " " << "roll" << view->f_roll << " " << "field" << view->f_field_of_view;
            libvlc_video_update_viewpoint(mp, view, true);
        }

        pos_mouse_ctrl_press_ = pos_event;
    }

}

void VLCDemo::wheelEvent(QWheelEvent *event)
{
    //qDebug() << "VLCDemo::wheelEvent";

    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.widget_play->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.widget_play->mapToGlobal(QPoint(ui.widget_play->width(), ui.widget_play->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);

    if (rect_preview.contains(pos_event))
    {
        if (event->delta() > 0) {                    // 当滚轮远离使用者时,进行放大


            view->f_field_of_view -= 3;
            if (view->f_field_of_view < 0)
            {
                view->f_field_of_view = 0;
            }

            qDebug() << "yaw" << view->f_yaw << " " << "pitch" << view->f_pitch << " " << "roll" << view->f_roll << " " << "field" << view->f_field_of_view;

            libvlc_video_update_viewpoint(mp, view, true);
        }
        else {                                     // 当滚轮向使用者方向旋转时,进行缩小
            view->f_field_of_view += 3;
            if (view->f_field_of_view > 180)
            {
                view->f_field_of_view = 180;
            }

            qDebug() << "yaw" << view->f_yaw << " " << "pitch" << view->f_pitch << " " << "roll" << view->f_roll << " " << "field" << view->f_field_of_view;

            libvlc_video_update_viewpoint(mp, view, true);
        }
    }
}
