package com.filetool;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;

/**
 * 报告生成器
 * 功能：1. 生成包含统计信息和重复文件的报告 2. 导出为TXT格式 3. 支持自定义报告路径
 */
public class ReportGenerator {

    /**
     * 生成完整报告并导出
     * @param scanner 扫描器对象（含统计数据）
     * @param duplicateMap 重复文件映射
     * @param reportPath 报告保存路径（如"C:\\报告.txt"）
     * @return 导出成功返回true，失败返回false
     */
    public boolean generateReport(FileScanner scanner,
                                  HashMap<String, ArrayList<File>> duplicateMap,
                                  String reportPath) {
        if (reportPath == null || reportPath.trim().isEmpty()) {
            System.out.println("❌ 错误：报告路径不能为空！");
            return false;
        }

        File reportFile = new File(reportPath);
        // 检查父目录是否存在，不存在则创建
        if (!reportFile.getParentFile().exists()) {
            reportFile.getParentFile().mkdirs(); // 递归创建目录
        }

        try (BufferedWriter writer = new BufferedWriter(new FileWriter(reportFile))) {
            // 报告头部（时间、标题）
            writer.write("==================== 文件检索与去重报告 ====================");
            writer.newLine();
            writer.write("生成时间：" + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date()));
            writer.newLine();
            writer.write("扫描目录：" + (GlobalData.fileList.isEmpty() ? "无" : GlobalData.fileList.get(0).getParent()));
            writer.newLine();
            writer.write("----------------------------------------------------------");
            writer.newLine();

            // 1. 统计信息
            writer.write("【文件统计信息】");
            writer.newLine();
            writer.write("  - 总文件数：" + scanner.getTotalFileCount() + " 个（已过滤0字节文件）");
            writer.newLine();
            writer.write("  - 总大小：" + formatSize(scanner.getTotalFileSize()));
            writer.newLine();
            writer.write("  - 类型分布：");
            writer.newLine();
            for (HashMap.Entry<String, Integer> entry : scanner.getFileTypeMap().entrySet()) {
                writer.write("    · " + entry.getKey() + "：" + entry.getValue() + " 个");
                writer.newLine();
            }
            writer.write("----------------------------------------------------------");
            writer.newLine();

            // 2. 重复文件信息
            writer.write("【重复文件信息】");
            writer.newLine();
            if (duplicateMap.isEmpty()) {
                writer.write("  - 未检测到重复文件");
            } else {
                writer.write("  - 重复文件组数：" + duplicateMap.size() + " 组");
                writer.newLine();
                int groupIndex = 1;
                for (HashMap.Entry<String, ArrayList<File>> entry : duplicateMap.entrySet()) {
                    writer.write("  第" + groupIndex + "组（MD5：" + entry.getKey() + "）：");
                    writer.newLine();
                    for (File file : entry.getValue()) {
                        writer.write("    · " + file.getAbsolutePath() + "（" + formatSize(file.length()) + "）");
                        writer.newLine();
                    }
                    groupIndex++;
                }
            }
            writer.write("==========================================================");
            writer.flush(); // 强制写入磁盘

            System.out.println("✅ 报告导出成功！路径：" + reportFile.getAbsolutePath());
            return true;

        } catch (Exception e) {
            System.out.println("❌ 报告导出失败！原因：" + e.getMessage());
            return false;
        }
    }

    /**
     * 格式化文件大小（复用扫描器的逻辑，避免代码冗余）
     */
    private String formatSize(long bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return String.format("%.1f KB", bytes / 1024.0);
        if (bytes < 1024 * 1024 * 1024) return String.format("%.1f MB", bytes / (1024.0 * 1024));
        return String.format("%.1f GB", bytes / (1024.0 * 1024 * 1024));
    }
}