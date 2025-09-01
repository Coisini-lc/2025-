from libs.PipeLine import PipeLine, ScopedTiming
from libs.YOLO import YOLOv5
import os,sys,gc
import ulab.numpy as np
import image
import time
from machine import UART
from machine import FPIOA
from machine import Pin

if __name__=="__main__":
    # 串口设置
    fpioa = FPIOA()
    fpioa.set_function(60, FPIOA.GPIO60)  # 配置GPIO60
    fpioa.set_function(5, FPIOA.UART2_TXD)  # 设置UART2的TX引脚
    fpioa.set_function(6, FPIOA.UART2_RXD)
    
    # LED初始化 - 初始状态为灭灯（低电平）
    LED_R = Pin(60, Pin.OUT, pull=Pin.PULL_NONE, drive=7)
    LED_R.low()  # 初始状态为低电平（灭灯）
    
    # LED闪烁控制变量
    led_blinking = False
    led_state = False  # 当前LED状态 (False=low/灭灯, True=high/亮灯)
    last_led_toggle_time = 0
    led_blink_interval = 500  # LED闪烁间隔，单位：毫秒
    
    # 串口设置
    u2 = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
    # UART write
    u2.write("UART2 Init successful\n")
    # 显示模式，默认"hdmi",可以选择"hdmi"和"lcd"
    display_mode="lcd"
    rgb888p_size=[1280,720]
    if display_mode=="hdmi":
        display_size=[1920,1080]
    else:
        display_size=[800,480]
    kmodel_path="/sdcard/fly/best.kmodel"
    labels = ["elephant","peacock","monkey","tiger","wolf"]
    confidence_threshold = 0.85
    nms_threshold=0.45
    model_input_size=[640,640]
    # 初始化PipeLine
    pl=PipeLine(rgb888p_size=rgb888p_size,display_size=display_size,display_mode=display_mode)
    
    # 添加摄像头初始化重试机制
    max_retries = 3
    retry_count = 0
    
    while retry_count < max_retries:
        try:
            pl.create(hmirror=True,vflip=True)
            print("摄像头初始化成功")
            break
        except Exception as camera_error:
            retry_count += 1
            print(f"摄像头初始化失败，第{retry_count}次重试: {camera_error}")
            if retry_count < max_retries:
                time.sleep(1)  # 等待1秒后重试
                gc.collect()   # 清理内存
            else:
                print("摄像头初始化失败，退出程序")
                sys.exit(1)
    # 初始化YOLOv5实例 - 修改为支持多目标检测
    yolo=YOLOv5(task_type="detect",mode="video",kmodel_path=kmodel_path,labels=labels,rgb888p_size=rgb888p_size,model_input_size=model_input_size,display_size=display_size,conf_thresh=confidence_threshold,nms_thresh=nms_threshold,max_boxes_num=50,debug_mode=0)
    yolo.config_preprocess()
    try:
        while True:
            # 检查串口是否有接收数据（非阻塞）
            if u2.any() > 0:
                received_data = u2.read()
                if received_data:
                    received_str = received_data.decode('utf-8', 'ignore')
                    if 'L' in received_str:
                        led_blinking = not led_blinking  # 切换LED闪烁状态
                        if led_blinking:
                            print("接收到'L'，开始LED闪烁")
                        else:
                            print("接收到'L'，停止LED闪烁")
                            LED_R.low()  # 停止闪烁时恢复为灭灯（低电平）
            
            # 非阻塞的LED闪烁逻辑
            if led_blinking:
                current_time = time.ticks_ms()
                if time.ticks_diff(current_time, last_led_toggle_time) >= led_blink_interval:
                    led_state = not led_state
                    if led_state:
                        LED_R.high()  # 亮灯
                    else:
                        LED_R.low()   # 灭灯
                    last_led_toggle_time = current_time
            
            os.exitpoint()
            with ScopedTiming("total",1):
                try:
                    # 逐帧推理
                    img=pl.get_frame()
                    res=yolo.run(img)
                    yolo.draw_result(res,pl.osd_img)
                    pl.show_image()
                except Exception as frame_error:
                    print(f"帧处理错误: {frame_error}")
                    gc.collect()  # 清理内存
                    continue  # 跳过这一帧，继续下一帧
                # 处理检测结果并发送到串口
                if res and len(res) == 3:  # 确保res有3个元素
                    bboxes = res[0]  # 边界框数组列表
                    class_ids = res[1]  # 类别ID列表
                    confidences = res[2]  # 置信度列表

                    if len(bboxes) > 0:  # 如果检测到目标
                        target_strings = []
                        sorted_detections = sorted(zip(bboxes, class_ids, confidences), key=lambda x: (x[0][0] + x[0][2]) / 2)
                        # 遍历所有检测到的对象
                        for i in range(len(bboxes)):
                            bbox = bboxes[i]  # 获取边界框
                            class_id = class_ids[i]  # 获取类别ID
                            confidence = confidences[i]  # 获取置信度
                            # 提取坐标 (x1, y1, x2, y2)
                            x1, y1, x2, y2 = bbox[0], bbox[1], bbox[2], bbox[3]

                            # 计算中心点坐标
                            center_x = int((x1 + x2) / 2)
                            center_y = int((y1 + y2) / 2)

                            # 格式化坐标为三位数，不足三位前面补0
                            center_x_str = f"{center_x:03d}"  # 格式化为3位数，不足补0
                            center_y_str = f"{center_y:03d}"  # 格式化为3位数，不足补0

                            # 构造输出字符串：类别序号+三位X坐标+三位Y坐标
                            target_str = f"{class_id}{center_x_str}{center_y_str}"
                            target_strings.append(target_str)

                        # 构造完整数据包：B + 数据 + T + 换行
                        data_content = ",".join(target_strings)
                        output_str = f"B{data_content}T\n"
                        u2.write(output_str)
                        print(f"发送: {output_str.strip()}")  # 调试输出
                    else:
                        # 没有检测到目标时发送空标识
                        pass
                gc.collect()

    except Exception as e:
        sys.print_exception(e)
    finally:
        u2.deinit()
        yolo.deinit()
        pl.destroy()
