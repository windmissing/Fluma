from pathlib import Path
p = Path('test/t476/gl_diff.cpp')
s = p.read_text(encoding='utf-8')
# disk 模式改为只校验（破坏由外部 python 在进程启动前完成）
old = '''    std::cout << "DISK_KEY=" << key << " file=" << file.string() << std::endl;
    assert(std::filesystem::exists(file));
    if (phase == "disk-truncate")
      std::filesystem::resize_file(file, 50);
    else {
      std::fstream bytes(file, std::ios::binary | std::ios::in | std::ios::out);
      bytes.seekp(144);
      bytes.put('x');
    }'''
new = '''    std::cout << "DISK_KEY=" << key << std::endl;
    // 破坏由外部 python 在本进程启动前完成（跨进程时序）；本进程只校验拒绝路由。
    assert(std::filesystem::exists(file));'''
assert old in s, 'disk block'
s = s.replace(old, new)
p.write_text(s, encoding='utf-8')
print('ok')
