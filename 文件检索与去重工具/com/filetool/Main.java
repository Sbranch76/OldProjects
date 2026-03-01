package com.filetool;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Scanner;

/**
 * 程序入口类
 * 功能：1. 提供用户交互菜单 2. 整合扫描、检测、报告、删除功能 3. 处理用户输入异常
 */
public class Main {
    public static void main(String[] args) {
        // 初始化核心功能对象（面向对象：每个类负责单一功能）
        FileScanner scanner = new FileScanner();
        DuplicateDetector detector = new DuplicateDetector();
        ReportGenerator reportGenerator = new ReportGenerator();
        Scanner userInput = new Scanner(System.in);

        // 主菜单循环
        while (true) {
            printMenu(); // 打印菜单
            int choice = getUserChoice(userInput); // 获取用户选择（带异常处理）

            switch (choice) {
                case 1:
                    // 1. 扫描目录
                    System.out.print("请输入要扫描的目录路径：");
                    String dirPath = userInput.nextLine().trim();
                    scanner.scanDirectory(dirPath);
                    break;

                case 2:
                    // 2. 检测重复文件
                    HashMap<String, ArrayList<File>> duplicateMap = detector.detectDuplicates(GlobalData.fileList);
                    printDuplicateSummary(duplicateMap); // 打印重复文件概要
                    break;

                case 3:
                    // 3. 生成报告
                    if (GlobalData.fileList.isEmpty()) {
                        System.out.println("❌ 请先执行【1. 扫描目录】！");
                        break;
                    }
                    HashMap<String, ArrayList<File>> dupMapForReport = detector.detectDuplicates(GlobalData.fileList);
                    System.out.print("请输入报告保存路径（如C:\\报告.txt）：");
                    String reportPath = userInput.nextLine().trim();
                    reportGenerator.generateReport(scanner, dupMapForReport, reportPath);
                    break;

                case 4:
                    // 4. 移除重复文件
                    if (GlobalData.fileList.isEmpty()) {
                        System.out.println("❌ 请先执行【1. 扫描目录】和【2. 检测重复文件】！");
                        break;
                    }
                    HashMap<String, ArrayList<File>> dupMapToRemove = detector.detectDuplicates(GlobalData.fileList);
                    int removed = detector.removeDuplicates(dupMapToRemove);
                    System.out.println("✅ 操作完成，共成功删除 " + removed + " 个重复文件");
                    break;

                case 5:
                    // 5. 退出程序
                    System.out.println("👋 程序已退出，感谢使用！");
                    userInput.close();
                    System.exit(0); // 正常退出

                default:
                    System.out.println("❌ 无效选择，请输入1-5之间的数字！");
            }
        }
    }

    /**
     * 打印主菜单
     */
    private static void printMenu() {
        System.out.println("\n===== 文件检索与去重工具 v1.0 =====");
        System.out.println("1. 扫描指定目录（统计文件信息）");
        System.out.println("2. 检测重复文件（基于内容MD5）");
        System.out.println("3. 生成并导出报告（TXT格式）");
        System.out.println("4. 移除重复文件（保留每组第一个）");
        System.out.println("5. 退出程序");
        System.out.print("请选择操作（1-5）：");
    }

    /**
     * 获取用户选择（处理非数字输入异常）
     * @param scanner 输入扫描器
     * @return 合法的选择（1-5）
     */
    private static int getUserChoice(Scanner scanner) {
        while (!scanner.hasNextInt()) {
            System.out.println("❌ 输入错误，请输入数字1-5！");
            scanner.next(); // 清空无效输入，避免死循环
            System.out.print("请重新选择：");
        }
        int choice = scanner.nextInt();
        scanner.nextLine(); // 吸收换行符，避免后续nextLine()读取空值
        return choice;
    }

    /**
     * 打印重复文件概要（仅显示文件名，避免路径过长）
     */
    private static void printDuplicateSummary(HashMap<String, ArrayList<File>> duplicateMap) {
        if (duplicateMap.isEmpty()) return;

        System.out.println("【重复文件概要】");
        int groupIndex = 1;
        for (ArrayList<File> dupFiles : duplicateMap.values()) {
            System.out.print("第" + groupIndex + "组（" + dupFiles.size() + "个文件）：");
            for (File file : dupFiles) {
                System.out.print(" " + file.getName());
            }
            System.out.println();
            groupIndex++;
        }
    }
}