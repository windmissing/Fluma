import cv2, os

videos = [
    r'D:\hmprojects\HelloWorld\tmp_cd1bee51b6822d34f511239cff2bccf2188dc9fde12bb707.mp4',
    r'D:\hmprojects\HelloWorld\tmp_6b849d7acace5f7317740f4079354138ce70d40a4ed00e5c.mp4',
]

out_dir = os.path.join(os.environ.get('TEMP', r'C:\Users\admin\AppData\Local\Temp'), 'frames')
os.makedirs(out_dir, exist_ok=True)

for i, vpath in enumerate(videos):
    cap = cv2.VideoCapture(vpath)
    total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    duration = total / fps if fps > 0 else 0
    print(f'Video {i+1}: {total} frames, {fps:.1f} fps, {w}x{h}, {duration:.1f}s')

    num_samples = 8
    for j in range(num_samples):
        frame_idx = int(total * j / num_samples)
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        ret, frame = cap.read()
        if ret:
            out_path = os.path.join(out_dir, f'video{i+1}_frame{j}.jpg')
            cv2.imwrite(out_path, frame)
            print(f'  Saved frame {j} at idx {frame_idx}')
    cap.release()
print('Done')