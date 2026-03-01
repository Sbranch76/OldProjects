package com.filetool;

import java.io.File;
import java.util.ArrayList;

/**
 * 全局数据共享类
 * 作用：存储扫描后的文件列表，供检测、报告模块直接使用，避免多类间参数传递
 */
public class GlobalData {
    // 静态集合：存储所有扫描到的有效文件（过滤0字节空文件）
    public static ArrayList<File> fileList = new ArrayList<>();
}