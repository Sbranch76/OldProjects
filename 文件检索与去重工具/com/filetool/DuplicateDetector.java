package com.filetool;

import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * 重复文件检测器
 * 功能：1. 计算文件MD5哈希值（基于内容） 2. 分组检测重复文件 3. 支持移除重复文件
 */
public class DuplicateDetector {

    /**
     * 检测所有文件中的重复项
     * @param fileList 待检测的文件列表
     * @return 重复文件映射（键：MD5哈希值，值：该哈希对应的所有重复文件）
     */
    public HashMap<String, ArrayList<File>> detectDuplicates(ArrayList<File> fileList) {
        if (fileList.isEmpty()) {
            System.out.println("❌ 无文件可检测，请先扫描目录！");
            return new HashMap<>();
        }

        System.out.println("\n🔍 开始检测重复文件（基于内容MD5）...");
        HashMap<String, ArrayList<File>> duplicateMap = new HashMap<>();

        for (File file : fileList) {
            String md5 = getFileMD5(file);
            if (md5 == null) continue; // 跳过计算失败的文件

            // 按MD5分组：同一MD5的文件放入同一个列表
            duplicateMap.computeIfAbsent(md5, k -> new ArrayList<>()).add(file);
        }

        // 过滤非重复项（仅保留文件数≥2的组）
        duplicateMap.entrySet().removeIf(entry -> entry.getValue().size() <= 1);

        System.out.println("✅ 检测完成，共发现 " + duplicateMap.size() + " 组重复文件");
        return duplicateMap;
    }

    /**
     * 计算文件的MD5哈希值（唯一标识文件内容）
     * @param file 待计算的文件
     * @return MD5哈希字符串（32位），计算失败返回null
     */
    public String getFileMD5(File file) {
        try (FileInputStream fis = new FileInputStream(file)) { // 自动关闭流，避免资源泄漏
            MessageDigest md = MessageDigest.getInstance("MD5");
            byte[] buffer = new byte[8192]; // 8KB缓冲区，高效读取大文件，提高程序稳定性，避免内存溢出
            int readLen;

            // 分块读取文件内容，更新哈希值
            while ((readLen = fis.read(buffer)) != -1) {
                md.update(buffer, 0, readLen);
            }

            // 哈希字节数组转16进制字符串
            byte[] digest = md.digest();
            StringBuilder md5Str = new StringBuilder();
            for (byte b : digest) {
                md5Str.append(String.format("%02x", b)); // 确保每个字节占2位（补0）
            }
            return md5Str.toString();

        } catch (Exception e) {
            System.out.println("⚠️ 计算MD5失败，文件：" + file.getName() + "，原因：" + e.getMessage());
            return null;
        }
    }

    /**
     * 移除重复文件（保留每组第一个文件，其他标记为可删除）
     * @param duplicateMap 重复文件映射
     * @return 成功移除的文件数
     */
    public int removeDuplicates(HashMap<String, ArrayList<File>> duplicateMap) {
        if (duplicateMap.isEmpty()) {
            System.out.println("❌ 无重复文件可移除！");
            return 0;
        }

        int removedCount = 0;
        for (ArrayList<File> dupFiles : duplicateMap.values()) {
            // 保留第一个文件，删除后续所有重复文件
            for (int i = 1; i < dupFiles.size(); i++) {
                File fileToRemove = dupFiles.get(i);
                if (fileToRemove.delete()) { // 物理删除文件
                    removedCount++;
                    System.out.println("🗑️ 已删除重复文件：" + fileToRemove.getAbsolutePath());
                } else {
                    System.out.println("⚠️ 无法删除文件：" + fileToRemove.getAbsolutePath() + "（可能无权限）");
                }
            }
        }
        return removedCount;
    }
}