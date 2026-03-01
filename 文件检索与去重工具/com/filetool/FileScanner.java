package com.filetool;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * 文件扫描器
 * 功能：1. 递归遍历指定目录及子目录 2. 统计文件数量、大小、类型 3. 过滤无效文件
 */
public class FileScanner {
    // 统计信息：文件总数、总大小（字节）、类型分布（键：后缀名，值：数量）
    private int totalFileCount;
    private long totalFileSize;
    private final HashMap<String, Integer> fileTypeMap = new HashMap<>();

    /**
     * 扫描指定目录入口方法
     * @param dirPath 目标目录路径
     * @return 扫描到的有效文件列表（非空文件）
     */
    public ArrayList<File> scanDirectory(String dirPath) {
        // 初始化统计数据（避免多次扫描时数据累加）
        resetStatistics();

        // 校验目录合法性
        File targetDir = new File(dirPath);
        if (!isValidDirectory(targetDir)) {
            return new ArrayList<>(); // 目录无效时返回空列表
        }

        // 递归扫描目录
        ArrayList<File> validFiles = new ArrayList<>();
        scanRecursive(targetDir, validFiles);//递归扫描

        // 更新全局数据
        GlobalData.fileList = validFiles;
        System.out.println("\n【扫描统计】");
        System.out.println("文件总数：" + totalFileCount + " 个（已过滤0字节文件）");
        System.out.println("总大小：" + formatSize(totalFileSize) + "（" + totalFileSize + " 字节）");
        System.out.println("类型分布：" + fileTypeMap);
        return validFiles;
    }

    /**
     * 递归遍历目录（私有方法，仅内部调用，隐藏实现细节）
     * @param currentDir 当前目录
     * @param resultList 存储有效文件的集合
     */
    private void scanRecursive(File currentDir, ArrayList<File> resultList) {
        File[] items = currentDir.listFiles();
        if (items == null) { // 无权限访问或空目录
            System.out.println("⚠️ 无权限访问目录，已跳过：" + currentDir.getAbsolutePath());
            return;
        }

        for (File item : items) {
            if (item.isDirectory()) {
                // 递归扫描子目录
                scanRecursive(item, resultList);
            } else if (item.isFile()) {
                // 过滤0字节空文件
                if (item.length() > 0) {
                    resultList.add(item);
                    // 更新统计数据
                    updateStatistics(item);
                }
            }
        }
    }

    /**
     * 校验目录是否合法（存在且为目录）
     * @param dir 待校验的目录
     * @return 合法返回true，否则打印错误信息并返回false
     */
    private boolean isValidDirectory(File dir) {
        if (!dir.exists()) {
            System.out.println("❌ 错误：目录不存在！路径：" + dir.getAbsolutePath());
            return false;
        }
        if (!dir.isDirectory()) {
            System.out.println("❌ 错误：指定路径不是目录！路径：" + dir.getAbsolutePath());
            return false;
        }
        return true;
    }

    /**
     * 更新文件统计信息（数量、大小、类型）
     * @param file 待统计的文件
     */
    private void updateStatistics(File file) {
        totalFileCount++; // 累计文件数
        totalFileSize += file.length(); // 累计大小

        // 提取文件后缀名（如".txt"），无后缀记为"[无后缀]"
        String fileName = file.getName();
        int dotIndex = fileName.lastIndexOf(".");
        String suffix = (dotIndex == -1) ? "[无后缀]" : fileName.substring(dotIndex).toLowerCase();

        // 更新类型分布（已存在则数量+1，否则初始化为1）
        fileTypeMap.put(suffix, fileTypeMap.getOrDefault(suffix, 0) + 1);
    }

    /**
     * 重置统计数据（多次扫描时使用）
     */
    private void resetStatistics() {
        totalFileCount = 0;
        totalFileSize = 0;
        fileTypeMap.clear();
    }

    /**
     * 格式化文件大小（字节→KB/MB/GB，提升可读性）
     * @param bytes 原始字节数
     * @return 格式化后的字符串（如"2.5 MB"）
     */
    private String formatSize(long bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return String.format("%.1f KB", bytes / 1024.0);
        if (bytes < 1024 * 1024 * 1024) return String.format("%.1f MB", bytes / (1024.0 * 1024));
        return String.format("%.1f GB", bytes / (1024.0 * 1024 * 1024));
    }

    // Getter方法：供报告模块获取统计数据
    public int getTotalFileCount() { return totalFileCount; }
    public long getTotalFileSize() { return totalFileSize; }
    public HashMap<String, Integer> getFileTypeMap() { return fileTypeMap; }
}