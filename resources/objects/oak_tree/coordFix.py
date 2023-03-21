import os

with open('oaktree.obj', 'r+') as fin:
    with open('oaktree_fix.obj', 'w+') as fout:
        for line in fin:
            print(line)
            if line[0] == 'v':
                coords = line.split()
                coords.pop(0)
                print(coords)
                coords = map(float, coords)
                coords = map(lambda x: x/1000, coords)

                coords = list(coords)
                fout.write("v {:.5f} {:.5f} {:.5f}\n".format(coords[0], coords[1], coords[2]))
            else:
                fout.write(line)
