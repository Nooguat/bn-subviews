import argparse

def lines_that_contains(pattern, fp):
    return [(line,idx)  for idx,line in enumerate(fp, 1) if pattern in line]



def list_parents(line : str):
    if line.count('\t') == 0:
        return None

def get_count(line:str):
    trimmed = line.lstrip(' ')
    return len(line) - len(trimmed)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('node')
    args = parser.parse_args()
    if args.node == None:
        exit()
    with open("qmainwindow.md", "r") as dump:
        file = dump.readlines()
        # TODO Find a way to not read the file twice
        dump.seek(0)
        matches = lines_that_contains(args.node, dump)
        parent = ""
        for match in matches:
            parent = match[0]
            index = match[1]
            print(parent, end=" ")
            while get_count(parent) > 0 and index > 0:
                if get_count(file[index]) < get_count(parent):
                    parent = file[index]
                    print(" -> ", parent, end=" ")
                index-=1
