#-*- encoding: utf-8 -*-
import json
import socket
import sys  
import os
import time
default_encoding = 'utf-8'
if sys.getdefaultencoding() != default_encoding:
   reload(sys)
   sys.setdefaultencoding(default_encoding)


dataA={
    "MessageType":"GetMaps",
    "Data": {}
}


dataB={
    "MessageType":"LoadMap",
    "Data": {
       "MapName":"LevelB"
    }
}

dataC={
    "MessageType":"GetActors",
    "Data": {
    }
}

dataD={
    "MessageType":"GetGetDetails",
    "Data": {
       "Path":"/Game/LevelA.LevelA:PersistentLevel.TestServer_2",
    }
}

dataE={
    "MessageType":"CallFunction",
    "Data": {
        "Path":"/Game/LevelA.LevelA:PersistentLevel.TestServer_2",
        "Function":"FunctionName",
        "PropertyPath":"PropertyPath",
        "FunctionParams":[],
    }
}

dataF={
    "MessageType":"SetProperty",
    "Data": {
        "Path":"/Game/LevelA.LevelA:PersistentLevel.TestServer_2",
        "PropertyArray":[
        {
        "Property":"aa",
        "Value":"True"
        },
        {
        "Property":"NewVar_1",
        "Value":"(X=5.000000,Y=6.000000,Z=7.000000)"
        }
        ]
    }
}

dataG={
    "MessageType":"SearchAssets",
    "Data": {
        "Query":"fe",
        "Class":"",
    }
}


def getmaps():
    s.send(json.dumps(dataA,ensure_ascii=False).encode("gbk")) 
    print(s.recv(102400).decode("utf-8"))

def loadmap(mapname):
    dataB["Data"]["MapName"]=mapname
    s.send(json.dumps(dataB,ensure_ascii=False).encode("gbk"))
    print(s.recv(102400).decode("utf-8"))

def getactors():
    s.send(json.dumps(dataC,ensure_ascii=False).encode("gbk")) 
    print(s.recv(102400).decode("utf-8"))

def getdetails(actorpath):
    dataD["Data"]["Path"]=actorpath
    s.send(json.dumps(dataD,ensure_ascii=False).encode("gbk")) 
    print(s.recv(102400).decode("utf-8"))

def callfunction(actor,function,param=[],propertyPath=""):
    dataE["Data"]["Path"]=actor
    dataE["Data"]["PropertyPath"]=propertyPath
    dataE["Data"]["Function"]=function
    dataE["Data"]["FunctionParams"]=param
    s.send(json.dumps(dataE,ensure_ascii=False).encode("utf-8"))
    print(s.recv(102400).decode("utf-8"))

def setproperty(actor,PropertyArray):
    dataF["Data"]["Path"]=actor
    dataF["Data"]["PropertyArray"]=PropertyArray
    s.send(json.dumps(dataF,ensure_ascii=False).encode("gbk"))
    print(s.recv(102400).decode("utf-8"))

def serachassets(query,classname):
    dataG["Data"]["Query"]=query
    dataG["Data"]["Class"]=classname
    s.send(json.dumps(dataG,ensure_ascii=False).encode("gbk"))
    print(s.recv(102400).decode("utf-8"))


def reconnect():
    global s
    s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 6162))
    os.system('cls')

s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)

reconnect()
# -----------------------------------------------------------------------------
# 获取地图
getmaps()

# 加载地图
loadmap("LevelA")
time.sleep(3)
loadmap("LevelB")
time.sleep(3)
loadmap("LevelA")
time.sleep(3)

# -----------------------------------------------------------------------------
# 加载地图后获取所有的Actor
getactors()

time.sleep(3)

# 获取单个Actor的详细信息 
getdetails("LevelA:PersistentLevel.TestServer_2")

time.sleep(3)
# -----------------------------------------------------------------------------
# 调用函数
FunctionParams=["(Pitch=6.000000,Yaw=0.000000,Roll=5.000000)","hello! i am 一个带空格  的字符串！！！"]
callfunction("LevelA:PersistentLevel.TestServer_2","中文函数",FunctionParams)

time.sleep(3)

# 设置属性变量
PropertyArray=[{"Property":"aa", "Value":"False"},{ "Property":"NewVar_1",  "Value":"(X=5.000000,Y=62.000000,Z=7.000000)"} ]
setproperty("LevelA:PersistentLevel.TestServer_2",PropertyArray)

time.sleep(3)
# -----------------------------------------------------------------------------
#关于位置的基本操作 Mobility=Movable 的Actor可以移动
# 设置坐标 Location
K2_SetActorLocation=["(X=0.000000,Y=0.000000,Z=0.000000)","False","False","{}","False"]
callfunction("LevelA:PersistentLevel.TestServer_2","K2_SetActorLocation",K2_SetActorLocation)
time.sleep(3)
# 设置旋转 Rotation
K2_SetActorRotation=["(Pitch=62.000000,Yaw=0.000000,Roll=5.000000)","True","","False"]
callfunction("LevelA:PersistentLevel.TestServer_2","K2_SetActorRotation",K2_SetActorRotation)

# 设置缩放 Scale
SetActorScale3D=["(X=1.000000,Y=2.000000,Z=2.000000)","False","False","{}","False"]
callfunction("LevelA:PersistentLevel.TestServer_2","SetActorScale3D",SetActorScale3D)

# 同时设置两个属性并调用函数显示
PropertyArray=[{"Property":"aa", "Value":"False"},{ "Property":"NewVar_1",  "Value":"(X=5.000000,Y=62.000000,Z=7.000000)"} ]
setproperty("LevelA:PersistentLevel.TestServer_2",PropertyArray)
callfunction("LevelA:PersistentLevel.TestServer_2","CustomEvent",[])
time.sleep(3)
# -----------------------------------------------------------------------------
# 设置Actor隐藏显示
# 隐藏
SetActorHiddenInGame=["True"]
callfunction("LevelA:PersistentLevel.Text3DActor_1","SetActorHiddenInGame",SetActorHiddenInGame)
time.sleep(3)
# 显示
SetActorHiddenInGame=["False"]
callfunction("LevelA:PersistentLevel.Text3DActor_1","SetActorHiddenInGame",SetActorHiddenInGame)

time.sleep(3)
# -----------------------------------------------------------------------------
# 关卡动画处理 isLevelSequenceActor=True类型的Actor有下列操作
# Play 等同Continue
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","Play",[],"SequencePlayer")
time.sleep(3)
# PlayLooping
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","PlayLooping",["-1"],"SequencePlayer")
time.sleep(3)
# Pause
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","Pause",[],"SequencePlayer")
time.sleep(3)
# Stop
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","Stop",[],"SequencePlayer")
time.sleep(3)
# JumpToSeconds
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","JumpToSeconds",["2.3f"],"SequencePlayer")
time.sleep(3)
# 从开始播 两个函数组合
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","JumpToSeconds",["0.0f"],"SequencePlayer")
callfunction("LevelA:PersistentLevel.NewLevelSequence_2","Play",[],"SequencePlayer")

time.sleep(3)
# -----------------------------------------------------------------------------
# 角色动画处理 Class=SkeletalMeshActor 类型的Actor有下列操作
# 查找动画资源
serachassets("","AnimationAsset")
time.sleep(3)
# 设置动画资源
AnimationAsset="AnimSequence'/Game/Mannequin/Animations/ThirdPerson_Jump.ThirdPerson_Jump'"
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","SetAnimation",[AnimationAsset],"SkeletalMeshComponent")
time.sleep(3)
AnimationAsset="AnimSequence'/Game/Mannequin/Animations/ThirdPersonWalk.ThirdPersonWalk'"
# 播放当前动画并设置Looping
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","Play",["True"],"SkeletalMeshComponent")
time.sleep(3)
# 设置动画同时播放动画并且设置Looping
AnimationAsset="AnimSequence'/Game/Mannequin/Animations/ThirdPerson_Jump.ThirdPerson_Jump'"
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","PlayAnimation",[AnimationAsset,"True"],"SkeletalMeshComponent")
time.sleep(3)
AnimationAsset="AnimSequence'/Game/Mannequin/Animations/ThirdPersonRun.ThirdPersonRun'"
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","PlayAnimation",[AnimationAsset,"True"],"SkeletalMeshComponent")
time.sleep(3)
# Stop
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","Stop",[],"SkeletalMeshComponent")
time.sleep(3)
# 设置播放速度 PlayRate
callfunction("LevelA:PersistentLevel.ThirdPersonJump_End_2","SetPlayRate",["2"],"SkeletalMeshComponent")

time.sleep(3)

# -----------------------------------------------------------------------------
# 文字
PropertyArray=[{"Property":"Text", "Value":"hello! i am Text"}]
setproperty("LevelA:PersistentLevel.TextRenderActor_1",PropertyArray)
# 3D文字
PropertyArray=[{"Property":"Text", "Value":"hello! i am 3D Text"} ]
setproperty("LevelA:PersistentLevel.Text3DActor_1",PropertyArray)

time.sleep(3)
def testnumber(n):
    for i in range(32):
        a= 1<<i
        if a&n:
            print(a)