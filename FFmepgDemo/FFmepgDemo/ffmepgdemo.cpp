#include "ffmepgdemo.h"

#include <QDebug>
#include <QFileDialog>
#include <windows.h>

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx = nullptr;
AVFilterContext *buffersrc_ctx = nullptr;
AVFilterInOut *outputs = nullptr;
AVFilterInOut *inputs = nullptr;
AVFilterGraph *filter_graph = nullptr;
static int video_stream_index = -1;

AVFrame *frame;
AVFrame *filt_frame;

//const char *filter_descr = "scale=78:24,transpose=cclock";
char *filter_descr = "v360=input=e:output=sg:yaw=-13:pitch=-94:roll=80:h_fov=320:v_fov=320";

QString filter_new;
QByteArray filter_ba = filter_descr;

static void PrintFfmpegError(int error_num)
{
    char buf[1024];
    av_strerror(error_num, buf, sizeof(buf));
    qDebug() << buf;
}

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;

    /* create decoding context */
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}


static int init_filters(const char *filters_descr)
{

    int ret = 0;
    clock_t cost = clock();

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    char args[512];
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE };

    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();


    if (buffersrc_ctx)
    {
        avfilter_free(buffersrc_ctx);
        buffersrc_ctx = nullptr;
    }
    if (buffersink_ctx)
    {
        avfilter_free(buffersink_ctx);
        buffersink_ctx = nullptr;
    }

    if (filter_graph)
    {
        qDebug() << "!filter_graph";
        avfilter_graph_free(&filter_graph);
        filter_graph = nullptr;
    }


    if (!filter_graph)
    {
        filter_graph = avfilter_graph_alloc();
        if (!outputs || !inputs || !filter_graph) {
            ret = AVERROR(ENOMEM);
            goto end;
        }
    }
    qDebug() << "avfilter_graph_alloc cost:" << clock() - cost;
    cost = clock();


    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
        time_base.num, time_base.den,
        dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    if (!buffersrc_ctx)
    {
        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
            args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
            goto end;
        }

        qDebug() << "avfilter_graph_create_filter in cost:" << clock() - cost;
        cost = clock();

        /* buffer video sink: to terminate the filter chain. */
        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
            NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            goto end;
        }
        
        qDebug() << "avfilter_graph_create_filter out cost:" << clock() - cost;
        cost = clock();

        ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            goto end;
        }

        qDebug() << "av_opt_set_int_list out cost:" << clock() - cost;
        cost = clock();
    }


    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
        &inputs, &outputs, NULL)) < 0)
    {
        PrintFfmpegError(ret);
        goto end;
    }

    qDebug() << "avfilter_graph_parse_ptr out cost:" << clock() - cost;
    cost = clock();

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
    {
        PrintFfmpegError(ret);
        goto end;
    }

    qDebug() << "avfilter_graph_config out cost:" << clock() - cost;
    cost = clock();

end:
//     if (inputs)
//     {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
//     }

    return ret;
}

FFmepgDemo::FFmepgDemo(QWidget *parent)
    : QMainWindow(parent),
    yaw_(0),
    pitch_(0),
    roll_(0),
    field_of_view_(230)
{
    ui.setupUi(this);

    connect(&timer, &QTimer::timeout, this, &FFmepgDemo::ShowTimerOut);

    
}

int FFmepgDemo::ChangeViewPoint()
{
    //char *filter_descr = "v360=input=e:output=sg:yaw=-13:pitch=-94:roll=80:h_fov=320:v_fov=320";

    filter_new = QString("v360=input=e:output=sg:yaw=%1:pitch=%2:roll=%3:h_fov=%4:v_fov=%4").arg(yaw_).arg(pitch_).arg(roll_).arg(field_of_view_);
    
    filter_ba = filter_new.toLocal8Bit();
    //filter_descr = filter_ba.data();

    return 0;
}

void FFmepgDemo::ShowTimerOut()
{
    int ret = 0;
    clock_t cost = clock();

    QString temp_filter = filter_descr;
    QString temp_filter_ba = filter_ba;

    

    if (temp_filter != temp_filter_ba)
    {
        filter_descr = filter_ba.data();
        if ((ret = init_filters(filter_descr)) < 0)
            return;
    }

    if (!buffersrc_ctx)
    {
        if ((ret = init_filters(filter_descr)) < 0)
            return;
    }

    qDebug() << "init_filter cost:" << clock() - cost;
    cost = clock();

    /* push the decoded frame into the filtergraph */
    if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return;
    }

    qDebug() << "av_buffersrc_add_frame_flags cost:" << clock() - cost;
    cost = clock();

    /* pull filtered frames from the filtergraph */
    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        return;
    if (ret < 0)
        return;

    qDebug() << "av_buffersink_get_frame cost:" << clock() - cost;
    cost = clock();

    //display_frame(filt_frame, buffersink_ctx->inputs[0]->time_base);
    int size = avpicture_get_size((AVPixelFormat)filt_frame->format, filt_frame->width, filt_frame->height);
    QImage* image = new QImage(filt_frame->data[0],
        filt_frame->width, filt_frame->height, QImage::Format_RGB888);

    av_frame_unref(filt_frame);

    QPixmap pix = QPixmap::fromImage(*image).scaled(ui.label_show->width() - 5,
        ui.label_show->height() - 5,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    //pix.loadFromData(filt_frame->data[0], size);
    ui.label_show->setPixmap(pix);

    delete image;
   
}

void FFmepgDemo::OpenPano(QString iamge_file)
{

    int ret;
    AVPacket packet;

    frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }

    if ((ret = open_input_file(iamge_file.toStdString().data())) < 0)
        goto end;


    if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
        goto end;

    if (packet.stream_index == video_stream_index) {
        ret = avcodec_send_packet(dec_ctx, &packet);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
            goto end;
        }

        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            goto end;
        }
        else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
            goto end;
        }

        frame->pts = frame->best_effort_timestamp;


        timer.start(1000);
        //         av_frame_unref(filt_frame);
        // 
        //         av_frame_unref(frame);
    }
    av_packet_unref(&packet);

end:
    //avfilter_graph_free(&filter_graph);
    //avcodec_free_context(&dec_ctx);
    //avformat_close_input(&fmt_ctx);
    //av_frame_free(&frame);
    //av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        //fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        //exit(1);
        qDebug() << "Error occurred:";
        PrintFfmpegError(ret);
    }

    //exit(0);
}

void FFmepgDemo::mousePressEvent(QMouseEvent *event)
{
    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.label_show->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.label_show->mapToGlobal(QPoint(ui.label_show->width(), ui.label_show->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);

    if (rect_preview.contains(pos_event))
    {
        pos_mouse_ctrl_press_ = pos_event;
    }
}

void FFmepgDemo::mouseReleaseEvent(QMouseEvent *event)
{

}

void FFmepgDemo::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "VLCDemo::mouseMoveEvent";

    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.label_show->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.label_show->mapToGlobal(QPoint(ui.label_show->width(), ui.label_show->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);


    if (rect_preview.contains(pos_event))
    {
        int x_move = pos_event.x() - pos_mouse_ctrl_press_.x();
        int y_move = pos_event.y() - pos_mouse_ctrl_press_.y();

        int move_abs = 10;
        if (move_abs < x_move || move_abs * (-1) > x_move)
        {
            int change_index = x_move / move_abs;
            yaw_ -= change_index;
            if (yaw_ > 180)
            {
                yaw_ = -180;
            }
            if (yaw_ < -180)
            {
                yaw_ = 180;
            }

            qDebug() << "yaw" << yaw_ << " " << "pitch" << pitch_ << " " << "roll" << roll_ << " " << "field" << field_of_view_;

            ChangeViewPoint();

        }

        if (move_abs < y_move || move_abs * (-1) > y_move)
        {
            int change_index = y_move / move_abs;
            pitch_ -= change_index;
            if (pitch_ > 180)
            {
                pitch_ = -180;
            }
            if (pitch_ < -180)
            {
                pitch_ = 180;
            }

            qDebug() << "yaw" << yaw_ << " " << "pitch" << pitch_ << " " << "roll" << roll_ << " " << "field" << field_of_view_;
            ChangeViewPoint();

        }

        pos_mouse_ctrl_press_ = pos_event;
    }

}

void FFmepgDemo::wheelEvent(QWheelEvent *event)
{
    //qDebug() << "VLCDemo::wheelEvent";

    QPoint pos_event = event->globalPos();
    QPoint pos_preview_topleft = ui.label_show->mapToGlobal(QPoint(0, 0));
    QPoint pos_preview_bottomright = ui.label_show->mapToGlobal(QPoint(ui.label_show->width(), ui.label_show->height()));
    QRect rect_preview = QRect(pos_preview_topleft, pos_preview_bottomright);

    if (rect_preview.contains(pos_event))
    {
        if (event->delta() > 0) {                    // 当滚轮远离使用者时,进行放大


            field_of_view_ -= 3;
            if (field_of_view_ < 0)
            {
                field_of_view_ = 0;
            }

            qDebug() << "yaw" << yaw_ << " " << "pitch" << pitch_ << " " << "roll" << roll_ << " " << "field" << field_of_view_;

            ChangeViewPoint();

        }
        else {                                     // 当滚轮向使用者方向旋转时,进行缩小
            field_of_view_ += 3;
            if (field_of_view_ > 180)
            {
                field_of_view_ = 180;
            }

            qDebug() << "yaw" << yaw_ << " " << "pitch" << pitch_ << " " << "roll" << roll_ << " " << "field" << field_of_view_;

            ChangeViewPoint();

        }
    }
}

void FFmepgDemo::on_pushbutton_open_clicked()
{
    QString iamge_file = QFileDialog::getOpenFileName(this, "", ".", "Images (*.png *.bmp *.jpg)");
    if (iamge_file.isEmpty())
    {
        return;
    }
//     image_.load(iamge_file);
//     image_ = image_.convertToFormat(QImage::Format_RGB888);
    //ChangeViewPoint();

    //timer.start(500);
    OpenPano(iamge_file);
}
