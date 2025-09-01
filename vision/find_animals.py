#!/usr/bin/env python3

import rospy
import serial
import re
from std_msgs.msg import String
from opencv_node.msg import off_data  # 自定义消息类型

class DualSerialNode:
    def __init__(self):
        rospy.init_node('dual_serial_node', anonymous=False)
        self.location = "A1B1"
        self.cx = rospy.get_param('~center_x', 190)
        self.cy = rospy.get_param('~center_y', 120)
        
        # 尝试智能检测拓展坞串口
        dock_port = self.get_dock_serial_port() or self.find_remaining_port(set())
        
        # 验证是否找到了拓展坞端口
        if not dock_port:
            rospy.logwarn("未检测到拓展坞串口，请手动指定 _port1 参数")
        
        # 串口1使用拓展坞
        port1 = rospy.get_param('~port1', dock_port)
        if port1:
            self.serial1 = self.init_serial(port1, rospy.get_param('~baudrate1', 115200), "串口1(拓展坞)")
        else:
            rospy.logerr("无法确定串口1端口，请手动指定")
            self.serial1 = None
        
        # 串口2使用已命名的设备
        self.serial2 = self.init_serial(rospy.get_param('~port2', '/dev/serial/by-id/usb-device1'), 
                                        rospy.get_param('~baudrate2', 115200), "串口2")
        
        self.data_pub = rospy.Publisher('/vision/off_data', off_data, queue_size=1)
        self.fcu_data_pub = rospy.Publisher('/setnofly', String, queue_size=1)
        rospy.Subscriber('/fcu_node/statue', String, self.state_callback)
        rospy.Subscriber('/fcu_node/path', String, self.string_array_callback)
        self.last_offset_x = [float('nan')] * 5
        self.last_offset_y = [float('nan')] * 5
        self.received_data = ""
        self.sent_locations = set()
        rospy.loginfo("节点初始化完成")

    def init_serial(self, port, baudrate, name):
        try:
            ser = serial.Serial(port=port, baudrate=baudrate, timeout=0.1)
            rospy.loginfo(f"{name}已连接: {port} @ {baudrate}")
            return ser
        except Exception as e:
            rospy.logerr(f"{name}连接失败: {e}")
            return None

    def get_dock_serial_port(self):
        """通过设备属性获取拓展坞串口"""
        import os
        import glob
        
        for device in glob.glob('/sys/class/tty/ttyUSB*/device'):
            try:
                # 读取设备制造商信息
                with open(f"{device}/../manufacturer", 'r') as f:
                    manufacturer = f.read().strip()
                
                # 读取产品信息  
                with open(f"{device}/../product", 'r') as f:
                    product = f.read().strip()
                
                # 根据制造商或产品名称判断
                if any(keyword in (manufacturer + product).lower() for keyword in 
                       ['dock', 'hub', 'expansion', 'station']):
                    
                    device_name = os.path.basename(os.path.dirname(device))
                    port = f"/dev/{device_name}"
                    rospy.loginfo(f"找到拓展坞串口: {port} ({manufacturer} {product})")
                    return port
                    
            except Exception:
                continue
        
        return None

    def find_remaining_port(self, known_ports):
        """找到除已知端口外的剩余端口"""
        import glob
        import os
        
        all_ports = glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')
        
        # 添加已知的命名设备路径
        known_named_devices = [
            '/dev/serial/by-id/usb-device1',
            '/dev/serial/by-id/usb-device2',
            # 添加其他已知设备
        ]
        
        # 解析已知端口的实际设备路径
        known_real_paths = set()
        for port in list(known_ports) + known_named_devices:
            try:
                real_path = os.path.realpath(port)
                known_real_paths.add(real_path)
            except:
                pass
        
        # 找到不在已知列表中的端口
        for port in all_ports:
            if port not in known_real_paths:
                rospy.loginfo(f"发现拓展坞候选端口: {port}")
                return port
        
        return None

    def state_callback(self, msg):
        loc = msg.data.strip()
        
        # 检查是否为 "landing" 指令
        if loc == "landing":
            rospy.loginfo("收到着陆指令，发送L到串口1")
            try:
                if self.serial1:
                    self.serial1.write(b'L')  # 发送单个字符L
                    rospy.loginfo("串口1已发送着陆指令: L")
                else:
                    rospy.logwarn("串口1未连接，无法发送着陆指令")
            except Exception as e:
                rospy.logerr(f"串口1发送着陆指令失败: {e}")
            return
        
        # 严格验证位置格式
        if re.fullmatch(r'A[1-9]B[1-7]', loc):
            old_location = self.location
            self.location = loc
            rospy.loginfo(f"位置已更新为: {self.location}")
            
            if old_location != self.location:
                rospy.loginfo(f"位置从 {old_location} 变更到 {self.location}")
                # 可选：如果希望每次位置变化时清除发送记录
                # self.sent_locations.clear()
                # rospy.loginfo("已清除所有位置的发送记录")
        else:
            rospy.logwarn(f"收到非法位置: {loc}，已忽略。有效范围：A1-A9, B1-B7 或 'landing'")

    def string_array_callback(self, msg):
        """接收长字符串数组并通过串口2转发"""
        string_data = msg.data.strip()
        if (string_data):
            # 去掉字符串中的所有空格
            string_data_no_spaces = string_data.replace(' ', '')
            # 添加帧头D和帧尾T
            send_str = f"D{string_data_no_spaces}T\n"
            try:
                if self.serial2:
                    self.serial2.write(send_str.encode('utf-8'))
                    rospy.loginfo(f"串口2转发字符串: {send_str.strip()}")
                else:
                    rospy.logwarn("串口2未连接，无法转发字符串")
            except Exception as e:
                rospy.logerr(f"串口2转发字符串失败: {e}")
        else:
            rospy.logwarn("接收到空字符串，跳过转发")

    def read_one_frame(self, ser):
        """更鲁棒的帧读取方法"""
        if not ser:
            return None
            
        timeout = rospy.Time.now() + rospy.Duration(2.0)  # 增加超时时间
        buffer = ""
        
        while rospy.Time.now() < timeout:
            try:
                if ser.in_waiting > 0:
                    # 读取所有可用数据
                    new_data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    buffer += new_data
                    rospy.logdebug(f"累积buffer: {buffer}")
                    
                    # 查找完整的B...T帧
                    while 'B' in buffer and 'T' in buffer:
                        b_index = buffer.find('B')
                        if b_index == -1:
                            break
                            
                        # 从B位置开始查找T
                        t_index = buffer.find('T', b_index)
                        if t_index == -1:
                            break
                            
                        # 提取完整帧
                        frame = buffer[b_index:t_index + 1]
                        buffer = buffer[t_index + 1:]  # 移除已处理的帧
                        
                        rospy.loginfo(f"读取到完整帧: {frame}")
                        return frame
                    
                    # 防止buffer过度增长
                    if len(buffer) > 500:
                        # 保留最后的部分，可能包含不完整的帧
                        buffer = buffer[-200:]
                        rospy.logwarn("Buffer过长，已清理")
                        
            except Exception as e:
                rospy.logerr(f"读取串口数据出错: {e}")
                break
                
            rospy.sleep(0.01)
        
        if buffer:
            rospy.logwarn(f"超时，未完成帧: {buffer}")
        return None

    def read_serial2_frame(self, ser):
        """读取串口2的A...T帧，返回去掉帧头帧尾的内容"""
        buffer = ""
        timeout = rospy.Time.now() + rospy.Duration(0.1)
        while rospy.Time.now() < timeout:
            if ser and ser.in_waiting > 0:
                char = ser.read(1).decode('utf-8', errors='ignore')
                buffer += char
                if buffer.startswith('A') and buffer.endswith('T'):
                    ser.reset_input_buffer()
                    # 去掉帧头A和帧尾T
                    data_content = buffer[1:-1]
                    rospy.loginfo(f"串口2接收到完整帧: {buffer.strip()}, 内容: {data_content}")
                    return data_content
                if len(buffer) > 50:  # 防止buffer过长
                    buffer = buffer[1:]
            rospy.sleep(0.001)
        return None

    def parse_data(self, data):
        # 匹配所有 {type}{xxx}{yyy} 组
        pattern = r'(\d)(\d{3})(\d{3})'
        matches = re.findall(pattern, data)
        offsets = []
        class_counts = {}
        for m in matches:
            try:
                object_type = int(m[0])
                x = int(m[1])
                y = int(m[2])
                dx = x - self.cx
                dy = y - self.cy
                offsets.append((object_type, dx, dy))
                class_counts[object_type] = class_counts.get(object_type, 0) + 1
                rospy.loginfo(f"目标解析: 类型={object_type}, x={x}, y={y}, dx={dx}, dy={dy}")
            except Exception as e:
                rospy.logwarn(f"解析目标失败: {m}, 错误: {str(e)}")
        rospy.loginfo(f"本帧共解析到{len(offsets)}个目标，类别统计: {class_counts}")
        return offsets, class_counts

    def send_to_serial2(self, class_counts):
        if not class_counts:
            rospy.loginfo("无目标数据，跳过串口2发送")
            return
        
        # 检查当前位置是否已经发送过数据
        if self.location in self.sent_locations:
            rospy.loginfo(f"位置 {self.location} 已发送过数据，跳过本次发送")
            return
        
        # 将所有目标数据组合成一个帧，每个类型都包含位置信息
        data_parts = []
        for cls, count in class_counts.items():
            data_parts.append(f"{cls}{count:d}{self.location}")
        
        # 用逗号连接各个类型的数据
        combined_data = ",".join(data_parts)
        send_str = f"A{combined_data}T\n"
        
        try:
            if self.serial2:
                self.serial2.write(send_str.encode('utf-8'))
                rospy.loginfo(f"串口2已发送: {send_str.strip()}")
            else:
                rospy.logwarn("串口2未连接，无法发送")
        except Exception as e:
            rospy.logerr(f"串口2发送失败: {e}")
            return  # 发送失败时不记录该位置
        
        # 记录该位置已发送过数据
        self.sent_locations.add(self.location)
        rospy.loginfo(f"位置 {self.location} 已记录为已发送状态")

    def publish_fcu_data(self, data_content):
        """发布串口2接收到的数据到/fcu_node/data话题"""
        msg = String()
        msg.data = data_content
        self.fcu_data_pub.publish(msg)
        rospy.loginfo(f"发布fcu_data: {data_content}")

    def publish_off_data(self, need_adjustment, offsets):
        msg = off_data()
        msg.need_adjustment = need_adjustment
        msg.num = len(offsets) if need_adjustment else 0
        if need_adjustment and offsets:
            msg.offset_x = [float(dx) for _, dx, _ in offsets][:5] + [float('nan')] * (5 - len(offsets))
            msg.offset_y = [float(dy) for _, _, dy in offsets][:5] + [float('nan')] * (5 - len(offsets))
            self.last_offset_x = msg.offset_x
            self.last_offset_y = msg.offset_y
        else:
            msg.offset_x = [float('nan')] * 5
            msg.offset_y = [float('nan')] * 5
        self.data_pub.publish(msg)
        rospy.loginfo(f"发布off_data: need_adjustment={msg.need_adjustment}, num={msg.num}")

    def spin(self):
        rate = rospy.Rate(10)
        loop_count = 0
        while not rospy.is_shutdown():
            offsets, class_counts = [], {}
            need_adjustment = False
            
            # 处理串口1数据（原有逻辑）
            if self.serial1 and self.serial1.in_waiting > 0:
                data = self.read_one_frame(self.serial1)
                if data:
                    offsets, class_counts = self.parse_data(data)
                    need_adjustment = len(offsets) > 0
                    if need_adjustment:
                        self.send_to_serial2(class_counts)
            else:
                loop_count += 1
                if loop_count % 50 == 0:
                    rospy.loginfo("串口1持续无数据")
            
            # 处理串口2接收数据（新增功能）
            if self.serial2 and self.serial2.in_waiting > 0:
                received_content = self.read_serial2_frame(self.serial2)
                if received_content:
                    self.received_data = received_content
                    self.publish_fcu_data(received_content)  # 接收到一次发布一次
            
            self.publish_off_data(need_adjustment, offsets)
            rate.sleep()

    def shutdown(self):
        if self.serial1:
            self.serial1.close()
            rospy.loginfo("串口1已关闭")
        if self.serial2:
            self.serial2.close()
            rospy.loginfo("串口2已关闭")
        rospy.loginfo("节点已关闭")

def main():
    try:
        node = DualSerialNode()
        rospy.on_shutdown(node.shutdown)
        node.spin()
    except rospy.ROSInterruptException:
        pass

if __name__ == '__main__':
    main()