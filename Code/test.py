import vision as vs

with vs.Image() as img:
    img.calibrate()
    found = False
    while not found:
        found,sign,tile = img.detect_sign([1,2,3,4,5,6,7,8,9])
    print('The sign that was found is {} in tile {}'.format(sign,tile))
