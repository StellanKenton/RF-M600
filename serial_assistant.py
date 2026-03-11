#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import threading
import time
from datetime import datetime
import struct

# 协议常量
PROTOCOL_HEADER_0 = 0x5A
PROTOCOL_HEADER_1 = 0xA5
PROTOCOL_DIR_HOST_TO_DEV = 0x00
PROTOCOL_DIR_DEV_TO_HOST = 0x01

# 模块定义
PROTOCOL_MODULE_ULTRASOUND = 0x01
PROTOCOL_MODULE_RADIO_FREQ = 0x02
PROTOCOL_MODULE_SHOCKWAVE = 0x03
PROTOCOL_MODULE_HEAT = 0x04

# 命令定义
PROTOCOL_CMD_GET_STATUS = 0x00
PROTOCOL_CMD_SET_WORK_STATE = 0x01
PROTOCOL_CMD_SET_CONFIG = 0x02

# 工作状态
WORK_STATE_STOP = 0x00
WORK_STATE_START = 0x01
WORK_STATE_RESET = 0x02

# 连接状态
CONN_STATE_CONNECTED_FOOT_CLOSED = 0x00
CONN_STATE_DISCONNECTED_FOOT_CLOSED = 0x10
CONN_STATE_CONNECTED_FOOT_OPEN = 0x01
CONN_STATE_DISCONNECTED_FOOT_OPEN = 0x11

# 温度 / 限值错误码
TEMP_ERROR_NTC_OPEN = 0xFFFF
TEMP_ERROR_NTC_SHORT = 0xEEFF
# 与固件一致：温度/频率等“超限”也用 0xFFFF 表示
TEMP_ERROR_OVER_LIMIT = 0xFFFF

# 配置结果
CONFIG_RESULT_SUCCESS = 0x00
CONFIG_RESULT_FAIL = 0x01
CONFIG_RESULT_OVER_LIMIT = 0x02


class ProtocolHelper:

    @staticmethod
    def crc16_compute(data):
        crc = 0x0000
        for byte in data:
            # 输入反转
            r = 0
            b = byte
            for i in range(8):
                r = (r << 1) | (b & 0x01)
                b >>= 1

            crc ^= (r << 8)
            crc &= 0xFFFF

            # 处理8位
            for i in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x8005) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF

        # 输出反转
        result = 0
        for i in range(16):
            result = (result << 1) | (crc & 0x01)
            crc >>= 1

        return result & 0xFFFF

    @staticmethod
    def build_packet(direction, module, cmd, data_bytes):
        packet = bytearray()
        packet.append(PROTOCOL_HEADER_0)
        packet.append(PROTOCOL_HEADER_1)
        packet.append(direction)
        packet.append(module)
        packet.append(cmd)
        packet.append(len(data_bytes))
        packet.extend(data_bytes)

        # 计算CRC16（只对数据部分）
        crc = ProtocolHelper.crc16_compute(data_bytes)
        packet.append(crc & 0xFF)
        packet.append((crc >> 8) & 0xFF)

        return bytes(packet)

    @staticmethod
    def parse_packet(data):
        if len(data) < 8:
            return None

        if data[0] != PROTOCOL_HEADER_0 or data[1] != PROTOCOL_HEADER_1:
            return None

        direction = data[2]
        module = data[3]
        cmd = data[4]
        data_len = data[5]

        if len(data) < 8 + data_len:
            return None

        payload = data[6:6+data_len]
        crc_recv = data[6+data_len] | (data[6+data_len+1] << 8)
        crc_calc = ProtocolHelper.crc16_compute(payload)

        if crc_recv != crc_calc:
            return None

        return {
            'direction': direction,
            'module': module,
            'cmd': cmd,
            'data_len': data_len,
            'payload': payload
        }


class SerialAssistant:

    def __init__(self, root):
        self.root = root
        self.root.title("RF-M600 串口调试助手")
        self.root.geometry("1000x800")

        self.serial_port = None
        self.is_connected = False
        self.receive_thread = None
        self.stop_receive = False

        self.setup_ui()
        self.refresh_ports()

    def setup_ui(self):

        # 顶部串口配置区域
        config_frame = ttk.Frame(self.root, padding="10")
        config_frame.pack(fill=tk.X)

        ttk.Label(config_frame, text="串口号:").grid(row=0, column=0, padx=5)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(config_frame, textvariable=self.port_var, width=15)
        self.port_combo.grid(row=0, column=1, padx=5)

        ttk.Button(config_frame, text="刷新", command=self.refresh_ports).grid(row=0, column=2, padx=5)

        ttk.Label(config_frame, text="波特率:").grid(row=0, column=3, padx=5)
        self.baudrate_var = tk.StringVar(value="115200")
        baudrate_combo = ttk.Combobox(config_frame, textvariable=self.baudrate_var,
                                     values=["9600", "19200", "38400", "57600", "115200", "230400"], width=10)
        baudrate_combo.grid(row=0, column=4, padx=5)

        self.connect_btn = ttk.Button(config_frame, text="打开串口", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=5, padx=5)

        # 主内容区域（左右分栏）
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # 左侧：命令发送区域
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)

        # 模块选择
        module_frame = ttk.LabelFrame(left_frame, text="模块选择", padding="10")
        module_frame.pack(fill=tk.X, pady=5)

        self.module_var = tk.IntVar(value=PROTOCOL_MODULE_ULTRASOUND)
        ttk.Radiobutton(module_frame, text="超声 (0x01)", variable=self.module_var,
                       value=PROTOCOL_MODULE_ULTRASOUND).pack(anchor=tk.W)
        ttk.Radiobutton(module_frame, text="射频 (0x02)", variable=self.module_var,
                       value=PROTOCOL_MODULE_RADIO_FREQ).pack(anchor=tk.W)
        ttk.Radiobutton(module_frame, text="冲击波 (0x03)", variable=self.module_var,
                       value=PROTOCOL_MODULE_SHOCKWAVE).pack(anchor=tk.W)
        ttk.Radiobutton(module_frame, text="热疗 (0x04)", variable=self.module_var,
                       value=PROTOCOL_MODULE_HEAT, command=self.on_module_change).pack(anchor=tk.W)
        self.module_var.trace('w', lambda *args: self.on_module_change())

        # 命令选择
        cmd_frame = ttk.LabelFrame(left_frame, text="命令选择", padding="10")
        cmd_frame.pack(fill=tk.X, pady=5)

        self.cmd_var = tk.IntVar(value=PROTOCOL_CMD_GET_STATUS)
        ttk.Radiobutton(cmd_frame, text="获取状态 (0x00)", variable=self.cmd_var,
                       value=PROTOCOL_CMD_GET_STATUS, command=self.on_cmd_change).pack(anchor=tk.W)
        ttk.Radiobutton(cmd_frame, text="设置工作状态 (0x01)", variable=self.cmd_var,
                       value=PROTOCOL_CMD_SET_WORK_STATE, command=self.on_cmd_change).pack(anchor=tk.W)
        ttk.Radiobutton(cmd_frame, text="设置配置 (0x02)", variable=self.cmd_var,
                       value=PROTOCOL_CMD_SET_CONFIG, command=self.on_cmd_change).pack(anchor=tk.W)
        self.cmd_var.trace('w', lambda *args: self.on_cmd_change())

        # 参数输入区域
        self.param_frame = ttk.LabelFrame(left_frame, text="参数输入", padding="10")
        self.param_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.param_widgets = {}
        self.setup_param_inputs()

        # 发送按钮
        send_frame = ttk.Frame(left_frame)
        send_frame.pack(fill=tk.X, pady=5)
        ttk.Button(send_frame, text="发送命令", command=self.send_command).pack(side=tk.LEFT, padx=5)
        ttk.Button(send_frame, text="清空参数", command=self.clear_params).pack(side=tk.LEFT, padx=5)

        # 右侧：数据收发显示区域
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=1)

        # 数据接收显示
        recv_frame = ttk.LabelFrame(right_frame, text="接收数据", padding="5")
        recv_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.recv_text = scrolledtext.ScrolledText(recv_frame, height=20, font=("Consolas", 9))
        self.recv_text.pack(fill=tk.BOTH, expand=True)

        # 数据发送显示
        send_display_frame = ttk.LabelFrame(right_frame, text="发送数据", padding="5")
        send_display_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.send_text = scrolledtext.ScrolledText(send_display_frame, height=10, font=("Consolas", 9))
        self.send_text.pack(fill=tk.BOTH, expand=True)

        # 清空按钮
        clear_frame = ttk.Frame(right_frame)
        clear_frame.pack(fill=tk.X, pady=5)
        ttk.Button(clear_frame, text="清空接收", command=lambda: self.recv_text.delete(1.0, tk.END)).pack(side=tk.LEFT, padx=5)
        ttk.Button(clear_frame, text="清空发送", command=lambda: self.send_text.delete(1.0, tk.END)).pack(side=tk.LEFT, padx=5)

    def setup_param_inputs(self):
        # 清除现有控件
        for widget in self.param_frame.winfo_children():
            widget.destroy()
        self.param_widgets.clear()

        module = self.module_var.get()
        cmd = self.cmd_var.get()

        # 根据模块和命令设置参数
        if cmd == PROTOCOL_CMD_GET_STATUS:
            # 获取状态命令通常无参数
            ttk.Label(self.param_frame, text="此命令无需参数", foreground="gray").pack(anchor=tk.W, pady=5)
        elif cmd == PROTOCOL_CMD_SET_WORK_STATE:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                self.add_param_input("工作状态", "work_state", "0-停止, 1-开始, 2-复位", "1", "uint8")
                self.add_param_input("工作时间(秒)", "work_time", "最大3600秒", "60", "uint16")
                self.add_param_input("工作级别", "work_level", "0-39 (40级)", "10", "uint8")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                self.add_param_input("工作状态", "work_state", "0-停止, 1-开始, 2-复位", "1", "uint8")
                self.add_param_input("工作时间(秒)", "work_time", "最大3600秒", "60", "uint16")
                self.add_param_input("工作级别", "work_level", "0-20", "10", "uint8")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                self.add_param_input("工作状态", "work_state", "0-停止, 1-开始, 2-复位", "1", "uint8")
                self.add_param_input("工作时间(秒)", "work_time", "最大3600秒", "60", "uint16")
                self.add_param_input("工作级别", "work_level", "0-26", "10", "uint8")
                self.add_param_input("频率", "frequency", "0-16", "8", "uint8")
            elif module == PROTOCOL_MODULE_HEAT:
                self.add_param_input("工作状态", "work_state", "0-停止, 1-开始, 2-复位", "1", "uint8")
                self.add_param_input("工作时间(秒)", "work_time", "最大3600秒", "60", "uint16")
                self.add_param_input("压力(KPa)", "pressure", "10-100", "50", "uint8")
                self.add_param_input("吸合时间(100ms)", "suck_time", "1-600 (0.1-60秒)", "10", "uint16")
                self.add_param_input("释放时间(100ms)", "release_time", "1-600 (0.1-60秒)", "10", "uint16")
                self.add_param_input("温度限制", "temp_limit", "350-480 (35-48℃)", "400", "uint16")
        elif cmd == PROTOCOL_CMD_SET_CONFIG:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                self.add_param_input("频率(kHz)", "frequency", "1000-1400", "1200", "uint16")
                self.add_param_input("电压(10mV)", "voltage", "1000-2000 (10-20V)", "1500", "uint16")
                self.add_param_input("温度限制", "temp_limit", "350-480 (35-48℃)", "400", "uint16")
                self.add_param_input("电流上限", "current_high_limit", "电流上限值", "1000", "uint16")
                self.add_param_input("电流下限", "current_low_limit", "电流下限值", "100", "uint16")
                self.add_param_input("剩余治疗次数", "remain_treatment_count", "剩余次数", "100", "uint16")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                self.add_param_input("温度限制", "temp_limit", "350-480 (35-48℃)", "400", "uint16")
                self.add_param_input("电流上限", "current_high_limit", "电流上限值", "1000", "uint16")
                self.add_param_input("电流下限", "current_low_limit", "电流下限值", "100", "uint16")
                self.add_param_input("剩余治疗次数", "remain_treatment_count", "剩余次数", "100", "uint16")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                self.add_param_input("温度限制", "temp_limit", "350-480 (35-48℃)", "400", "uint16")
                self.add_param_input("ESW-P电流上限", "esw_p_current_high_limit", "ESW-P电流上限", "1000", "uint16")
                self.add_param_input("ESW-P电流下限", "esw_p_current_low_limit", "ESW-P电流下限", "100", "uint16")
                self.add_param_input("剩余治疗次数", "remain_treatment_count", "剩余次数", "100", "uint16")
                self.add_param_input("ESW-N电流上限", "esw_n_current_high_limit", "ESW-N电流上限", "1000", "uint16")
                self.add_param_input("ESW-N电流下限", "esw_n_current_low_limit", "ESW-N电流下限", "100", "uint16")
            elif module == PROTOCOL_MODULE_HEAT:
                self.add_param_input("预热状态", "preheat_state", "0-停止, 1-开始", "1", "uint8")
                self.add_param_input("工作时间(秒)", "work_time", "最大3600秒", "300", "uint16")
                self.add_param_input("温度限制", "temp_limit", "350-480 (35-48℃)", "400", "uint16")
                self.add_param_input("预热温度限制", "preheat_temp_limit", "350-480 (35-48℃)", "400", "uint16")
                self.add_param_input("剩余治疗次数", "remain_treatment_count", "剩余次数", "100", "uint16")

    def add_param_input(self, label, key, desc, default, data_type):
        frame = ttk.Frame(self.param_frame)
        frame.pack(fill=tk.X, pady=2)

        # 参数说明
        desc_label = ttk.Label(frame, text=f"{label}: {desc}", font=("Arial", 9))
        desc_label.pack(anchor=tk.W)

        # 输入框
        input_frame = ttk.Frame(frame)
        input_frame.pack(fill=tk.X, padx=20)

        entry = ttk.Entry(input_frame, width=20)
        entry.insert(0, default)
        entry.pack(side=tk.LEFT, padx=5)

        self.param_widgets[key] = {
            'entry': entry,
            'type': data_type,
            'label': label
        }

    def on_module_change(self, *args):
        self.setup_param_inputs()

    def on_cmd_change(self, *args):
        self.setup_param_inputs()

    def clear_params(self):
        self.setup_param_inputs()

    def get_param_value(self, key):
        if key not in self.param_widgets:
            return None

        widget = self.param_widgets[key]
        value_str = widget['entry'].get().strip()

        if not value_str:
            return None

        try:
            if widget['type'] == 'uint8':
                return int(value_str) & 0xFF
            elif widget['type'] == 'uint16':
                return int(value_str) & 0xFFFF
            else:
                return int(value_str)
        except ValueError:
            return None

    def build_command_data(self):
        module = self.module_var.get()
        cmd = self.cmd_var.get()
        data_bytes = bytearray()

        if cmd == PROTOCOL_CMD_GET_STATUS:
            # 获取状态命令，通常只有一个dummy字节0x00
            data_bytes.append(0x00)
        elif cmd == PROTOCOL_CMD_SET_WORK_STATE:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                v = self.get_param_value('work_state')
                work_state = v if v is not None else 1
                work_time = self.get_param_value('work_time') or 60
                work_level = self.get_param_value('work_level') or 10
                data_bytes.append(work_state)
                data_bytes.append(work_time & 0xFF)
                data_bytes.append((work_time >> 8) & 0xFF)
                data_bytes.append(work_level)
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                v = self.get_param_value('work_state')
                work_state = v if v is not None else 1
                work_time = self.get_param_value('work_time') or 60
                work_level = self.get_param_value('work_level') or 10
                data_bytes.append(work_state)
                data_bytes.append(work_time & 0xFF)
                data_bytes.append((work_time >> 8) & 0xFF)
                data_bytes.append(work_level)
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                v = self.get_param_value('work_state')
                work_state = v if v is not None else 1
                work_time = self.get_param_value('work_time') or 60
                work_level = self.get_param_value('work_level') or 10
                frequency = self.get_param_value('frequency') or 8
                data_bytes.append(work_state)
                data_bytes.append(work_time & 0xFF)
                data_bytes.append((work_time >> 8) & 0xFF)
                data_bytes.append(work_level)
                data_bytes.append(frequency)
            elif module == PROTOCOL_MODULE_HEAT:
                v = self.get_param_value('work_state')
                work_state = v if v is not None else 1
                work_time = self.get_param_value('work_time') or 60
                pressure = self.get_param_value('pressure') or 50
                suck_time = self.get_param_value('suck_time') or 10
                release_time = self.get_param_value('release_time') or 10
                temp_limit = self.get_param_value('temp_limit') or 400
                data_bytes.append(work_state)
                data_bytes.append(work_time & 0xFF)
                data_bytes.append((work_time >> 8) & 0xFF)
                data_bytes.append(pressure)
                data_bytes.append(suck_time & 0xFF)
                data_bytes.append((suck_time >> 8) & 0xFF)
                data_bytes.append(release_time & 0xFF)
                data_bytes.append((release_time >> 8) & 0xFF)
                data_bytes.append(temp_limit & 0xFF)
                data_bytes.append((temp_limit >> 8) & 0xFF)
        elif cmd == PROTOCOL_CMD_SET_CONFIG:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                frequency = self.get_param_value('frequency') or 1200
                voltage = self.get_param_value('voltage') or 1500
                temp_limit = self.get_param_value('temp_limit') or 400
                current_high_limit = self.get_param_value('current_high_limit') or 1000
                current_low_limit = self.get_param_value('current_low_limit') or 100
                remain_treatment_count = self.get_param_value('remain_treatment_count') or 100
                data_bytes.append(frequency & 0xFF)
                data_bytes.append((frequency >> 8) & 0xFF)
                data_bytes.append(voltage & 0xFF)
                data_bytes.append((voltage >> 8) & 0xFF)
                data_bytes.append(temp_limit & 0xFF)
                data_bytes.append((temp_limit >> 8) & 0xFF)
                data_bytes.append(current_high_limit & 0xFF)
                data_bytes.append((current_high_limit >> 8) & 0xFF)
                data_bytes.append(current_low_limit & 0xFF)
                data_bytes.append((current_low_limit >> 8) & 0xFF)
                data_bytes.append(remain_treatment_count & 0xFF)
                data_bytes.append((remain_treatment_count >> 8) & 0xFF)
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                temp_limit = self.get_param_value('temp_limit') or 400
                current_high_limit = self.get_param_value('current_high_limit') or 1000
                current_low_limit = self.get_param_value('current_low_limit') or 100
                remain_treatment_count = self.get_param_value('remain_treatment_count') or 100
                data_bytes.append(temp_limit & 0xFF)
                data_bytes.append((temp_limit >> 8) & 0xFF)
                data_bytes.append(current_high_limit & 0xFF)
                data_bytes.append((current_high_limit >> 8) & 0xFF)
                data_bytes.append(current_low_limit & 0xFF)
                data_bytes.append((current_low_limit >> 8) & 0xFF)
                data_bytes.append(remain_treatment_count & 0xFF)
                data_bytes.append((remain_treatment_count >> 8) & 0xFF)
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                temp_limit = self.get_param_value('temp_limit') or 400
                esw_p_current_high_limit = self.get_param_value('esw_p_current_high_limit') or 1000
                esw_p_current_low_limit = self.get_param_value('esw_p_current_low_limit') or 100
                remain_treatment_count = self.get_param_value('remain_treatment_count') or 100
                esw_n_current_high_limit = self.get_param_value('esw_n_current_high_limit') or 1000
                esw_n_current_low_limit = self.get_param_value('esw_n_current_low_limit') or 100
                data_bytes.append(temp_limit & 0xFF)
                data_bytes.append((temp_limit >> 8) & 0xFF)
                data_bytes.append(esw_p_current_high_limit & 0xFF)
                data_bytes.append((esw_p_current_high_limit >> 8) & 0xFF)
                data_bytes.append(esw_p_current_low_limit & 0xFF)
                data_bytes.append((esw_p_current_low_limit >> 8) & 0xFF)
                data_bytes.append(remain_treatment_count & 0xFF)
                data_bytes.append((remain_treatment_count >> 8) & 0xFF)
                data_bytes.append(esw_n_current_high_limit & 0xFF)
                data_bytes.append((esw_n_current_high_limit >> 8) & 0xFF)
                data_bytes.append(esw_n_current_low_limit & 0xFF)
                data_bytes.append((esw_n_current_low_limit >> 8) & 0xFF)
            elif module == PROTOCOL_MODULE_HEAT:
                pv = self.get_param_value('preheat_state')
                preheat_state = pv if pv is not None else 1
                work_time = self.get_param_value('work_time') or 300
                temp_limit = self.get_param_value('temp_limit') or 400
                preheat_temp_limit = self.get_param_value('preheat_temp_limit') or 400
                remain_treatment_count = self.get_param_value('remain_treatment_count') or 100
                data_bytes.append(preheat_state)
                data_bytes.append(work_time & 0xFF)
                data_bytes.append((work_time >> 8) & 0xFF)
                data_bytes.append(temp_limit & 0xFF)
                data_bytes.append((temp_limit >> 8) & 0xFF)
                data_bytes.append(preheat_temp_limit & 0xFF)
                data_bytes.append((preheat_temp_limit >> 8) & 0xFF)
                data_bytes.append(remain_treatment_count & 0xFF)
                data_bytes.append((remain_treatment_count >> 8) & 0xFF)

        return bytes(data_bytes)

    def send_command(self):
        if not self.is_connected:
            messagebox.showwarning("警告", "请先打开串口！")
            return

        try:
            module = self.module_var.get()
            cmd = self.cmd_var.get()
            data_bytes = self.build_command_data()

            packet = ProtocolHelper.build_packet(
                PROTOCOL_DIR_HOST_TO_DEV,
                module,
                cmd,
                data_bytes
            )

            self.serial_port.write(packet)

            # 显示发送的数据
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            module_name = self.get_module_name(module)
            cmd_name = self.get_cmd_name(cmd)

            hex_str = ' '.join([f'{b:02X}' for b in packet])
            self.send_text.insert(tk.END, f"[{timestamp}] 发送 - {module_name} - {cmd_name}\n")
            self.send_text.insert(tk.END, f"数据: {hex_str}\n")
            self.send_text.insert(tk.END, f"长度: {len(packet)} 字节\n")
            self.send_text.insert(tk.END, f"参数: {self.format_send_params(module, cmd, data_bytes)}\n")
            self.send_text.insert(tk.END, "-" * 60 + "\n")
            self.send_text.see(tk.END)

        except Exception as e:
            messagebox.showerror("错误", f"发送失败: {str(e)}")

    def format_send_params(self, module, cmd, data_bytes):
        if cmd == PROTOCOL_CMD_GET_STATUS:
            return "无参数"

        params = []
        idx = 0

        if cmd == PROTOCOL_CMD_SET_WORK_STATE:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                params.append(f"工作状态={data_bytes[idx]}")
                params.append(f"工作时间={data_bytes[idx+1] | (data_bytes[idx+2] << 8)}秒")
                params.append(f"工作级别={data_bytes[idx+3]}")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                params.append(f"工作状态={data_bytes[idx]}")
                params.append(f"工作时间={data_bytes[idx+1] | (data_bytes[idx+2] << 8)}秒")
                params.append(f"工作级别={data_bytes[idx+3]}")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                params.append(f"工作状态={data_bytes[idx]}")
                params.append(f"工作时间={data_bytes[idx+1] | (data_bytes[idx+2] << 8)}秒")
                params.append(f"工作级别={data_bytes[idx+3]}")
                params.append(f"频率={data_bytes[idx+4]}")
            elif module == PROTOCOL_MODULE_HEAT:
                params.append(f"工作状态={data_bytes[idx]}")
                params.append(f"工作时间={data_bytes[idx+1] | (data_bytes[idx+2] << 8)}秒")
                params.append(f"压力={data_bytes[idx+3]}KPa")
                params.append(f"吸合时间={data_bytes[idx+4] | (data_bytes[idx+5] << 8)}*100ms")
                params.append(f"释放时间={data_bytes[idx+6] | (data_bytes[idx+7] << 8)}*100ms")
                params.append(f"温度限制={data_bytes[idx+8] | (data_bytes[idx+9] << 8)}")
        elif cmd == PROTOCOL_CMD_SET_CONFIG:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                params.append(f"频率={data_bytes[idx] | (data_bytes[idx+1] << 8)}kHz")
                params.append(f"电压={data_bytes[idx+2] | (data_bytes[idx+3] << 8)}*10mV")
                params.append(f"温度限制={data_bytes[idx+4] | (data_bytes[idx+5] << 8)}")
                params.append(f"电流上限={data_bytes[idx+6] | (data_bytes[idx+7] << 8)}")
                params.append(f"电流下限={data_bytes[idx+8] | (data_bytes[idx+9] << 8)}")
                params.append(f"剩余治疗次数={data_bytes[idx+10] | (data_bytes[idx+11] << 8)}")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                params.append(f"温度限制={data_bytes[idx] | (data_bytes[idx+1] << 8)}")
                params.append(f"电流上限={data_bytes[idx+2] | (data_bytes[idx+3] << 8)}")
                params.append(f"电流下限={data_bytes[idx+4] | (data_bytes[idx+5] << 8)}")
                params.append(f"剩余治疗次数={data_bytes[idx+6] | (data_bytes[idx+7] << 8)}")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                params.append(f"温度限制={data_bytes[idx] | (data_bytes[idx+1] << 8)}")
                params.append(f"ESW-P电流上限={data_bytes[idx+2] | (data_bytes[idx+3] << 8)}")
                params.append(f"ESW-P电流下限={data_bytes[idx+4] | (data_bytes[idx+5] << 8)}")
                params.append(f"剩余治疗次数={data_bytes[idx+6] | (data_bytes[idx+7] << 8)}")
                params.append(f"ESW-N电流上限={data_bytes[idx+8] | (data_bytes[idx+9] << 8)}")
                params.append(f"ESW-N电流下限={data_bytes[idx+10] | (data_bytes[idx+11] << 8)}")
            elif module == PROTOCOL_MODULE_HEAT:
                params.append(f"预热状态={data_bytes[idx]}")
                params.append(f"工作时间={data_bytes[idx+1] | (data_bytes[idx+2] << 8)}秒")
                params.append(f"温度限制={data_bytes[idx+3] | (data_bytes[idx+4] << 8)}")
                params.append(f"预热温度限制={data_bytes[idx+5] | (data_bytes[idx+6] << 8)}")
                params.append(f"剩余治疗次数={data_bytes[idx+7] | (data_bytes[idx+8] << 8)}")

        return ", ".join(params)

    def parse_received_data(self, packet_info):
        module = packet_info['module']
        cmd = packet_info['cmd']
        payload = packet_info['payload']

        result = []
        result.append(f"模块: {self.get_module_name(module)}")
        result.append(f"命令: {self.get_cmd_name(cmd)}")
        result.append(f"数据长度: {len(payload)} 字节")
        result.append("")

        if cmd == PROTOCOL_CMD_GET_STATUS:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                if len(payload) >= 14:
                    work_state = payload[0]
                    frequency = payload[1] | (payload[2] << 8)
                    temp_limit = payload[3] | (payload[4] << 8)
                    remain_time = payload[5] | (payload[6] << 8)
                    work_level = payload[7]
                    head_temp = payload[8] | (payload[9] << 8)
                    conn_state = payload[10]
                    error_code = payload[11]
                    remain_treatment_count = payload[12] | (payload[13] << 8)

                    result.append(f"工作状态: {self.get_work_state_name(work_state)}")
                    result.append(f"频率: {frequency} kHz")
                    result.append(f"温度限制: {self.format_temp(temp_limit)}")
                    result.append(f"剩余时间: {remain_time} 秒")
                    result.append(f"工作级别: {work_level}")
                    result.append(f"头部温度: {self.format_temp_value(head_temp)}")
                    result.append(f"连接状态: {self.get_conn_state_name(conn_state)}")
                    result.append(f"错误码: 0x{error_code:02X}")
                    result.append(f"剩余治疗次数: {remain_treatment_count}")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                if len(payload) >= 12:
                    work_state = payload[0]
                    temp_limit = payload[1] | (payload[2] << 8)
                    remain_time = payload[3] | (payload[4] << 8)
                    work_level = payload[5]
                    head_temp = payload[6] | (payload[7] << 8)
                    conn_state = payload[8]
                    error_code = payload[9]
                    remain_treatment_count = payload[10] | (payload[11] << 8)

                    result.append(f"工作状态: {self.get_work_state_name(work_state)}")
                    result.append(f"温度限制: {self.format_temp(temp_limit)}")
                    result.append(f"剩余时间: {remain_time} 秒")
                    result.append(f"工作级别: {work_level}")
                    result.append(f"头部温度: {self.format_temp_value(head_temp)}")
                    result.append(f"连接状态: {self.get_conn_state_name(conn_state)}")
                    result.append(f"错误码: 0x{error_code:02X}")
                    result.append(f"剩余治疗次数: {remain_treatment_count}")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                if len(payload) >= 11:
                    work_state = payload[0]
                    frequency = payload[1]
                    remain_time = payload[2] | (payload[3] << 8)
                    work_level = payload[4]
                    head_temp = payload[5] | (payload[6] << 8)
                    conn_state = payload[7]
                    error_code = payload[8]
                    remain_treatment_count = payload[9] | (payload[10] << 8)

                    result.append(f"工作状态: {self.get_work_state_name(work_state)}")
                    result.append(f"频率: {frequency} 级")
                    result.append(f"剩余时间: {remain_time} 秒")
                    result.append(f"工作级别: {work_level}")
                    result.append(f"头部温度: {self.format_temp_value(head_temp)}")
                    result.append(f"连接状态: {self.get_conn_state_name(conn_state)}")
                    result.append(f"错误码: 0x{error_code:02X}")
                    result.append(f"剩余治疗次数: {remain_treatment_count}")
            elif module == PROTOCOL_MODULE_HEAT:
                if len(payload) >= 21:
                    work_state = payload[0]
                    temp_limit = payload[1] | (payload[2] << 8)
                    remain_heat_time = payload[3] | (payload[4] << 8)
                    suck_time = payload[5] | (payload[6] << 8)
                    release_time = payload[7] | (payload[8] << 8)
                    pressure = payload[9]
                    head_temp = payload[10] | (payload[11] << 8)
                    preheat_state = payload[12]
                    preheat_temp_limit = payload[13] | (payload[14] << 8)
                    remain_preheat_time = payload[15] | (payload[16] << 8)
                    conn_state = payload[17]
                    error_code = payload[18]
                    remain_treatment_count = payload[19] | (payload[20] << 8)

                    result.append(f"工作状态: {self.get_work_state_name(work_state)}")
                    result.append(f"温度限制: {self.format_temp(temp_limit)}")
                    result.append(f"剩余加热时间: {remain_heat_time} 秒")
                    result.append(f"吸合时间: {suck_time}*10ms")
                    result.append(f"释放时间: {release_time}*10ms")
                    result.append(f"压力: {pressure} KPa")
                    result.append(f"头部温度: {self.format_temp_value(head_temp)}")
                    result.append(f"预热状态: {self.get_work_state_name(preheat_state)}")
                    result.append(f"预热温度限制: {self.format_temp(preheat_temp_limit)}")
                    result.append(f"剩余预热时间: {remain_preheat_time} 秒")
                    result.append(f"连接状态: {self.get_conn_state_name(conn_state)}")
                    result.append(f"错误码: 0x{error_code:02X}")
                    result.append(f"剩余治疗次数: {remain_treatment_count}")
        elif cmd == PROTOCOL_CMD_SET_CONFIG:
            if module == PROTOCOL_MODULE_ULTRASOUND:
                if len(payload) >= 6:
                    freq_result = payload[0]
                    voltage_result = payload[1]
                    temp_result = payload[2]
                    current_high_result = payload[3]
                    current_low_result = payload[4]
                    remain_treatment_count_result = payload[5]
                    result.append(f"频率配置结果: {self.get_config_result_name(freq_result)}")
                    result.append(f"电压配置结果: {self.get_config_result_name(voltage_result)}")
                    result.append(f"温度配置结果: {self.get_config_result_name(temp_result)}")
                    result.append(f"电流上限配置结果: {self.get_config_result_name(current_high_result)}")
                    result.append(f"电流下限配置结果: {self.get_config_result_name(current_low_result)}")
                    result.append(f"剩余治疗次数配置结果: {self.get_config_result_name(remain_treatment_count_result)}")
            elif module == PROTOCOL_MODULE_RADIO_FREQ:
                if len(payload) >= 4:
                    temp_result = payload[0]
                    current_high_result = payload[1]
                    current_low_result = payload[2]
                    remain_treatment_count_result = payload[3]
                    result.append(f"温度配置结果: {self.get_config_result_name(temp_result)}")
                    result.append(f"电流上限配置结果: {self.get_config_result_name(current_high_result)}")
                    result.append(f"电流下限配置结果: {self.get_config_result_name(current_low_result)}")
                    result.append(f"剩余治疗次数配置结果: {self.get_config_result_name(remain_treatment_count_result)}")
            elif module == PROTOCOL_MODULE_SHOCKWAVE:
                if len(payload) >= 6:
                    temp_result = payload[0]
                    esw_p_current_high_result = payload[1]
                    esw_p_current_low_result = payload[2]
                    remain_treatment_count_result = payload[3]
                    esw_n_current_high_result = payload[4]
                    esw_n_current_low_result = payload[5]
                    result.append(f"温度配置结果: {self.get_config_result_name(temp_result)}")
                    result.append(f"ESW-P电流上限配置结果: {self.get_config_result_name(esw_p_current_high_result)}")
                    result.append(f"ESW-P电流下限配置结果: {self.get_config_result_name(esw_p_current_low_result)}")
                    result.append(f"剩余治疗次数配置结果: {self.get_config_result_name(remain_treatment_count_result)}")
                    result.append(f"ESW-N电流上限配置结果: {self.get_config_result_name(esw_n_current_high_result)}")
                    result.append(f"ESW-N电流下限配置结果: {self.get_config_result_name(esw_n_current_low_result)}")
            elif module == PROTOCOL_MODULE_HEAT:
                if len(payload) >= 5:
                    preheat_state_result = payload[0]
                    work_time_result = payload[1]
                    temp_limit_result = payload[2]
                    preheat_temp_limit_result = payload[3]
                    remain_treatment_count_result = payload[4]
                    result.append(f"预热状态配置结果: {self.get_config_result_name(preheat_state_result)}")
                    result.append(f"工作时间配置结果: {self.get_config_result_name(work_time_result)}")
                    result.append(f"温度限制配置结果: {self.get_config_result_name(temp_limit_result)}")
                    result.append(f"预热温度限制配置结果: {self.get_config_result_name(preheat_temp_limit_result)}")
                    result.append(f"剩余治疗次数配置结果: {self.get_config_result_name(remain_treatment_count_result)}")

        return "\n".join(result)

    def get_module_name(self, module):
        names = {
            PROTOCOL_MODULE_ULTRASOUND: "超声",
            PROTOCOL_MODULE_RADIO_FREQ: "射频",
            PROTOCOL_MODULE_SHOCKWAVE: "冲击波",
            PROTOCOL_MODULE_HEAT: "热疗"
        }
        return names.get(module, f"未知模块(0x{module:02X})")

    def get_cmd_name(self, cmd):
        names = {
            PROTOCOL_CMD_GET_STATUS: "获取状态",
            PROTOCOL_CMD_SET_WORK_STATE: "设置工作状态",
            PROTOCOL_CMD_SET_CONFIG: "设置配置"
        }
        return names.get(cmd, f"未知命令(0x{cmd:02X})")

    def get_work_state_name(self, state):
        names = {
            WORK_STATE_STOP: "停止",
            WORK_STATE_START: "工作",
            WORK_STATE_RESET: "复位"
        }
        return names.get(state, f"未知(0x{state:02X})")

    def get_conn_state_name(self, state):
        names = {
            CONN_STATE_CONNECTED_FOOT_CLOSED: "头部连接，脚部关闭",
            CONN_STATE_DISCONNECTED_FOOT_CLOSED: "头部断开，脚部关闭",
            CONN_STATE_CONNECTED_FOOT_OPEN: "头部连接，脚部打开",
            CONN_STATE_DISCONNECTED_FOOT_OPEN: "头部断开，脚部打开"
        }
        return names.get(state, f"未知(0x{state:02X})")

    def get_config_result_name(self, result):
        names = {
            CONFIG_RESULT_SUCCESS: "成功",
            CONFIG_RESULT_FAIL: "失败",
            CONFIG_RESULT_OVER_LIMIT: "超限"
        }
        return names.get(result, f"未知(0x{result:02X})")

    def format_temp(self, value):
        if value == TEMP_ERROR_OVER_LIMIT:
            return "超限"
        return f"{value/10:.1f}℃"

    def format_temp_value(self, value):
        if value == TEMP_ERROR_NTC_OPEN:
            return "NTC开路"
        elif value == TEMP_ERROR_NTC_SHORT:
            return "NTC短路"
        else:
            return f"{value/10:.1f}℃"

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.port_combo['values'] = port_list
        if port_list and not self.port_var.get():
            self.port_var.set(port_list[0])

    def toggle_connection(self):
        if not self.is_connected:
            self.open_port()
        else:
            self.close_port()

    def open_port(self):
        try:
            port = self.port_var.get()
            if not port:
                messagebox.showwarning("警告", "请选择串口！")
                return

            baudrate = int(self.baudrate_var.get())
            self.serial_port = serial.Serial(port, baudrate, timeout=0.1)
            self.is_connected = True
            self.connect_btn.config(text="关闭串口")
            self.port_combo.config(state='disabled')

            # 启动接收线程
            self.stop_receive = False
            self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
            self.receive_thread.start()

            self.recv_text.insert(tk.END, f"[{datetime.now().strftime('%H:%M:%S')}] 串口已打开: {port} @ {baudrate}\n")
            self.recv_text.see(tk.END)

        except Exception as e:
            messagebox.showerror("错误", f"打开串口失败: {str(e)}")

    def close_port(self):
        try:
            self.stop_receive = True
            if self.serial_port:
                self.serial_port.close()
                self.serial_port = None
            self.is_connected = False
            self.connect_btn.config(text="打开串口")
            self.port_combo.config(state='normal')

            self.recv_text.insert(tk.END, f"[{datetime.now().strftime('%H:%M:%S')}] 串口已关闭\n")
            self.recv_text.see(tk.END)

        except Exception as e:
            messagebox.showerror("错误", f"关闭串口失败: {str(e)}")

    def receive_data(self):
        buffer = bytearray()

        while not self.stop_receive and self.is_connected:
            try:
                if self.serial_port and self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer.extend(data)

                    # 尝试解析数据包
                    while len(buffer) >= 8:
                        # 查找帧头
                        header_idx = -1
                        for i in range(len(buffer) - 1):
                            if buffer[i] == PROTOCOL_HEADER_0 and buffer[i+1] == PROTOCOL_HEADER_1:
                                header_idx = i
                                break

                        if header_idx == -1:
                            # 没找到帧头，清空缓冲区
                            buffer.clear()
                            break

                        if header_idx > 0:
                            # 丢弃帧头之前的数据
                            buffer = buffer[header_idx:]

                        if len(buffer) < 8:
                            break

                        # 检查方向（应该是设备到主机）
                        if buffer[2] != PROTOCOL_DIR_DEV_TO_HOST:
                            buffer.pop(0)
                            continue

                        data_len = buffer[5]
                        packet_len = 8 + data_len

                        if len(buffer) < packet_len:
                            # 数据不完整，等待更多数据
                            break

                        # 提取完整数据包
                        packet_data = bytes(buffer[:packet_len])
                        buffer = buffer[packet_len:]

                        # 解析数据包
                        packet_info = ProtocolHelper.parse_packet(packet_data)
                        if packet_info:
                            # 在主线程中更新UI
                            self.root.after(0, self.display_received_data, packet_data, packet_info)
                        else:
                            # CRC校验失败，丢弃一个字节继续
                            if len(buffer) > 0:
                                buffer.pop(0)

                time.sleep(0.01)

            except Exception as e:
                if self.is_connected:
                    self.root.after(0, lambda: messagebox.showerror("错误", f"接收数据出错: {str(e)}"))
                break

    def display_received_data(self, packet_data, packet_info):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        hex_str = ' '.join([f'{b:02X}' for b in packet_data])

        self.recv_text.insert(tk.END, f"[{timestamp}] 接收数据\n")
        self.recv_text.insert(tk.END, f"原始数据: {hex_str}\n")
        self.recv_text.insert(tk.END, f"长度: {len(packet_data)} 字节\n")
        self.recv_text.insert(tk.END, "\n")
        self.recv_text.insert(tk.END, self.parse_received_data(packet_info))
        self.recv_text.insert(tk.END, "\n")
        self.recv_text.insert(tk.END, "-" * 60 + "\n")
        self.recv_text.see(tk.END)


def main():
    root = tk.Tk()
    app = SerialAssistant(root)
    root.mainloop()


if __name__ == "__main__":
    main()
