# Untitled - By: dave - Sat Oct 28 2017

import sensor

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames()
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking
sensor.set_windowing((100,100, 120, 20))
thresholds = [(0, 0, -0, -0, -0, -3)] # Either blue or green tape.

while(True):
    img = sensor.snapshot()
    img.gaussian(2, unsharp=True)
    lines = img.find_lines(threshold=100)
    #for line in img.find_lines(threshold=100):
    #    img.draw_line(line.x1(), line.y1(), line.x2(), line.y2(), 0, 2)
    if len(lines) == 2:
        left = min(lines[0].x1(), lines[1].x1())
        right = max(lines[0].x1(), lines[1].x1())
        if (right - left) < 30 and right - left > 15:
            print(right - left)
            stats = img.get_statistics(roi = (left, 0, right - left, 20))
            print(stats)
            thesholds = (stats.l_lq(), stats.l_uq(), stats.a_lq(), stats.a_uq(), stats.b_lq(), stats.b_uq())
    for blob in img.find_blobs(thresholds, x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8,merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())

