from pathlib import Path
p = Path('test/t476/store_diff.cpp')
s = p.read_text(encoding='utf-8')
# F24 重写：分批 Drain 防队列满；keys[1] 可能已被驱逐（touched 最旧），
# 用 keys 数组末尾（最新 touched）的 key 做 receipt 目标更稳；jsonl 文件本身
# 不依赖 entry 存在（Receipt 按文件写）。用 keys[1] 保持——jsonl 文件存在即可。
old = '''    // receipt 历史滚动
    for (int i = 0; i < 400; ++i)
      s.Receipt(keys[1], "{\\"stage\\":\\"ROLL\\",\\"i\\":" + std::to_string(i) + "}");
    s.Drain();
    auto rl = Read(dir / (keys[1] + ".jsonl"));
    check("F24_RECEIPT_ROLL_BOUNDED", rl.size() <= kReceiptLimit && !rl.empty(),
          "receipt 历史滚动 ≤128KiB");'''
new = '''    // receipt 历史滚动（1500 条 ≈150KB > 128KiB 触发滚动；每 100 条 Drain 防队列满）
    for (int i = 0; i < 1500; ++i) {
      s.Receipt(keys[1], "{\\"stage\\":\\"ROLL\\",\\"i\\":" + std::to_string(i) + "}");
      if (i % 100 == 99)
        s.Drain();
    }
    s.Drain();
    auto rl = Read(dir / (keys[1] + ".jsonl"));
    check("F24_RECEIPT_ROLL_BOUNDED", rl.size() <= kReceiptLimit && !rl.empty(),
          "receipt 历史滚动 ≤128KiB（写 " + std::to_string(rl.size()) + "B）");'''
assert old in s, 'F24'
s = s.replace(old, new)
p.write_text(s, encoding='utf-8')
print('ok')
