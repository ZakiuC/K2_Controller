#!/usr/bin/env python3
"""
WebSocket测试脚本
用于测试K2_Controller的WebSocket功能
"""

import asyncio
import websockets
import json
import time

async def test_websocket():
    uri = "ws://115.190.42.239:8080"
    
    try:
        async with websockets.connect(uri) as websocket:
            print(f"已连接到 {uri}")
            
            # 测试系统状态查询
            test_msg = {
                "type": "system",
                "command": "get_status",
                "deviceId": "",
                "data": ""
            }
            
            print(f"发送消息: {test_msg}")
            await websocket.send(json.dumps(test_msg))
            
            # 接收响应
            response = await websocket.recv()
            print(f"收到响应: {response}")
            
            # 测试设备列表查询
            test_msg2 = {
                "type": "system", 
                "command": "get_devices",
                "deviceId": "",
                "data": ""
            }
            
            print(f"发送消息: {test_msg2}")
            await websocket.send(json.dumps(test_msg2))
            
            response2 = await websocket.recv()
            print(f"收到响应: {response2}")
            
            # 测试设备命令
            test_msg3 = {
                "type": "command",
                "command": "0x00",
                "deviceId": "motor_4",
                "data": "00 00 00 00 64 00 00 00"
            }
            
            print(f"发送设备命令: {test_msg3}")
            await websocket.send(json.dumps(test_msg3))
            
            response3 = await websocket.recv()
            print(f"收到响应: {response3}")
            
    except Exception as e:
        print(f"连接错误: {e}")

if __name__ == "__main__":
    print("WebSocket测试脚本")
    print("测试服务器: 115.190.42.239:8080")
    print("=" * 50)
    
    asyncio.run(test_websocket())
