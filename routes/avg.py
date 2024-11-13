data = [[( int(e) if e.strip().isdigit() else (1 if e == "DAY" else 0) )for e in d.strip().split("\t")] for d in open("route.txt").readlines()[1:]]


from collections import defaultdict

d = defaultdict(lambda: [0,0])

for i in data:
    d[i[0]][0] += 1
    d[i[0]][1] += i[1]


out = []

for i in d.keys():
    out.append([i, d[i][1]/d[i][0] ])


