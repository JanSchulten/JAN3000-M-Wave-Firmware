#!/bin/bash

# 脚本名称: repack_libs.sh (v2 - 无 -C 参数支持)
# 功能: 处理当前目录下所有的静态库 (.a 文件), 使用 lto-ar.exe 解压再用 rcs 重新打包 (不使用 -C 参数)

# 检查 lto-ar.exe 是否存在
if ! command -v lto-ar.exe &> /dev/null
then
    echo "错误: lto-ar.exe 命令未找到。请确保它在 PATH 环境变量中或者与脚本在同一目录下。"
    exit 1
fi

# 查找当前目录下所有的 .a 文件
find . -maxdepth 1 -name "*.a" -print0 | while IFS= read -r -d $'\0' library_file; do
    if [[ -f "$library_file" ]]; then
        library_name=$(basename "$library_file" .a)
        temp_dir="./temp_${library_name}_dir"
        original_dir=$(pwd) # 保存原始目录

        echo "处理库文件: $library_file"

        # 1. 创建临时目录
        mkdir -p "$temp_dir"
        if [ $? -ne 0 ]; then
            echo "  创建临时目录 $temp_dir 失败!"
            continue # 继续处理下一个库
        fi
        echo "  创建临时目录: $temp_dir"

        # 2. 切换到临时目录
        echo "  切换到临时目录: $temp_dir"
        pushd "$temp_dir" > /dev/null # 切换目录并压入栈，静默输出
        if [ $? -ne 0 ]; then
            echo "  切换到临时目录失败!"
            popd > /dev/null # 尝试返回，忽略错误
            rm -rf "$temp_dir" # 清理临时目录
            continue # 继续处理下一个库
        fi

        # 3. 解压库文件到当前 (临时) 目录 (不使用 -C)
        echo "  解压库文件到临时目录 (当前目录)..."
        lto-ar.exe x "../${library_file}" # 注意库文件路径需要相对原始目录
        if [ $? -ne 0 ]; then
            echo "  解压库文件失败!"
            popd > /dev/null # 返回原始目录，忽略错误
            rm -rf "$temp_dir" # 清理临时目录
            continue # 继续处理下一个库
        fi

        # 4. 返回到原始目录
        echo "  返回到原始目录"
        popd > /dev/null # 返回原始目录，静默输出
        if [ $? -ne 0 ]; then
            echo "  返回原始目录失败 (但库文件可能已解压在临时目录)! 请手动检查和清理。"
            rm -rf "$temp_dir" # 清理临时目录 (尝试)
            continue # 继续处理下一个库
        fi


        # 5. 备份原始库文件
        backup_file="${library_file}.orig"
        if [ -e "$backup_file" ]; then
            rm -f "$backup_file" # 如果备份文件已存在，先删除
        fi
        mv "$library_file" "$backup_file"
        if [ $? -ne 0 ]; then
            echo "  备份原始库文件失败!"
            rm -rf "$temp_dir" # 清理临时目录
            continue # 继续处理下一个库
        fi
        echo "  原始库文件已备份为: $backup_file"

        # 6. 使用 rcs 重新打包库文件
        echo "  使用 lto-ar.exe rcs 重新打包库文件..."
        lto-ar.exe rcs "$library_file" "$temp_dir"/*
        if [ $? -ne 0 ]; then
            echo "  重新打包库文件失败!"
            mv "$backup_file" "$library_file" # 恢复原始库文件
            rm -rf "$temp_dir" # 清理临时目录
            continue # 继续处理下一个库
        fi

        # 7. 清理临时目录
        echo "  清理临时目录: $temp_dir"
        rm -rf "$temp_dir"
        if [ $? -ne 0 ]; then
            echo "  清理临时目录失败 (但库文件已重新打包)!"
        fi

        echo "  库文件 $library_file 处理完成 (使用 rcs 重新打包)."
        echo "----------------------------------------"
    fi
done

echo "所有库文件处理完成."

exit 0