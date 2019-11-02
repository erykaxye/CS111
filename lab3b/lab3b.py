#!/usr/bin/python

#NAME: Erica Xie, Michelle Lam
#EMAIL: ericaxie@ucla.edu, laammichelle@gmail.com
#ID: 404920875, 404926523

import sys
import os
import string

#Global initialize variables
inodes = {}
dir = []
indir = []
free_inodes = []
free_blocks = []
inode_state = {}
block_state = {}
links = []
parent = []
in_use = {}

num_blocks = 0
first_legal_block = 5
first_nonreserved_ino = 0
num_inodes = 0
status = 0

def is_legal_block(block):
    global num_blocks, first_legal_block
    if (block > num_blocks or block < first_legal_block):
        return False
    else:
        return True

def is_free_block(block):
    global free_blocks
    if(not is_legal_block(block)):
        return False
    return free_blocks[block]

def is_free_inode(inode):
    global first_nonreserved_ino, num_inodes
    if ((2 < inode < first_nonreserved_ino) or (inode > num_inodes)):
        return False
    return free_inodes[inode - 1]

def check_block(block):
    global num_blocks, block_state
    if(block < 0 or block > num_blocks):
        return 'INVALID'

    s = block_state.get(block)
    if(s == 'RESERVED' or s == 'FREE' or s == None):
        return s
    else:
        return 'IN USE'

def scan_inodes(inode):
    global inode_state, block_state, inodes, in_use, status
    temp = inodes.get(inode)
    # If it is a file, it should have at least 1 link
    # Allocated inode
    if(inode_state[inode] > 0):
        if free_inodes[inode - 1] != 0:
            print('ALLOCATED INODE ' + str(inode) + ' ON FREELIST')
            status = 1
        # No block pointers
        if (int(temp[10]) < 60 and temp[2] == 's'):
            return
        # Has block pointers
        elif ((temp[2] == 'd') or (temp[2] == 'f') or ((temp[2] == 's') and temp[10] > 60)):
            for i in range (1, 16):
                ptr = int(temp[11 + i])
                s = None
                if(ptr != 0):
                    s = check_block(ptr)
                if(s == None):
                    block_state[ptr] = 'IN USE'
                    if (i == 13):
                        in_use[ptr] = str('DUPLICATE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 12')
                    elif (i == 14):
                        in_use[ptr] = str('DUPLICATE DOUBLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 268')
                    elif (i == 15):
                        in_use[ptr] = str('DUPLICATE TRIPLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 65804')
                    else:
                        in_use[ptr] = str('DUPLICATE BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 0')
                elif(s == 'IN USE'):
                    block_state[ptr] = 'DUPLICATE'
                    if (i == 13):
                        print(in_use[ptr])
                        print('DUPLICATE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 12')
                        status = 1
                    elif (i == 14):
                        print(in_use[ptr])
                        print('DUPLICATE DOUBLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 268')
                        status = 1
                    elif (i == 15):
                        print(in_use[ptr])
                        print('DUPLICATE TRIPLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 65804')
                        status = 1
                    else:
                        print(in_use[ptr])
                        print('DUPLICATE BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 0')
                        status = 1
                elif(s == 'FREE'):
                    print('ALLOCATED BLOCK ' + str(ptr) + ' ON FREELIST')
                    status = 1
                elif (s == 'INVALID' or s == 'RESERVED'):
                    if (i == 13):
                        print(s + ' INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 12')
                        status = 1
                    elif (i == 14):
                        print(s + ' DOUBLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 268')
                        status = 1
                    elif (i == 15):
                        print(s + ' TRIPLE INDIRECT BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 65804')
                        status = 1
                    else:
                        print(s + ' BLOCK ' + str(ptr) + ' IN INODE ' + str(inode) + ' AT OFFSET 0')
                        status = 1
    # Unallocated inode - first 10 inodes are reserved
    elif (inode_state[inode] == 0 and inode > 10):
        if (free_inodes[inode - 1] == 0):
            print('UNALLOCATED INODE ' + str(inode) + ' NOT ON FREELIST')
            status = 1

def scan_indir(ilist):
    global in_use, status
    temp = int(ilist[5])
    level = int(ilist[2])
    inode = int(ilist[1])
    s = None
    s = check_block(temp)
    if(s == None):
        block_state[temp] = 'IN USE'
        if (level == 1):
            in_use[temp] = str('DUPLICATE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 12')
        elif (level == 2):
            in_use[temp] = str('DUPLICATE DOUBLE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 268')
        elif (level == 3):
            in_use[temp] = str('DUPLICATE TRIPLE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 65804')
        else:
            in_use[temp] = str('DUPLICATE BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 0')
    elif(s == 'IN USE'):
        block_state[temp] = 'DUPLICATE'
        if (level == 1):
            print(in_use[temp])
            print('DUPLICATE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 12')
            status = 1
        elif (level == 2):
            print(in_use[temp])
            print('DUPLICATE DOUBLE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 268')
            status = 1
        elif (level == 3):
            print(in_use[temp])
            print('DUPLICATE TRIPLE INDIRECT BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 65804')
            status = 1
        else:
            print(in_use[temp])
            print('DUPLICATE BLOCK ' + str(temp) + ' IN INODE ' + str(inode) + ' AT OFFSET 0')
            status = 1

def scan_dir(dir_list):
    global num_inodes, inode_state, links, parent, status
    parent_inode = int(dir_list[1])
    ref_inode = int(dir_list[3])
    file_name = dir_list[6]
    # Invalid and unallocated inode checks
    if (ref_inode < 1 or ref_inode > num_inodes):
        print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + file_name + ' INVALID INODE ' + str(ref_inode))
        status = 1
    elif (inode_state.get(ref_inode) == 0):
        print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + file_name + ' UNALLOCATED INODE ' + str(ref_inode))
        status = 1
    else:
        links[ref_inode - 1] += 1
        if((file_name != "'.'") or (file_name != "'..'") and (int(parent[ref_inode - 1]) == 0)):
            parent[ref_inode - 1] = parent_inode

def scan_dir_2(dir_list):
    global num_inodes, inode_state, links, parent, status
    parent_inode = int(dir_list[1])
    ref_inode = int(dir_list[3])
    file_name = dir_list[6]
    if(file_name == "'.'" and ref_inode != parent_inode):
        print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + file_name + ' LINK TO INODE ' + str(ref_inode) + ' SHOULD BE ' + str(parent_inode))
        status = 1
    if (file_name == "'..'"):
        if(parent_inode == 2 and ref_inode != parent_inode):
            print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + file_name + ' LINK TO INODE ' + str(ref_inode) + ' SHOULD BE ' + str(parent_inode))
            status = 1
        if(parent_inode != 2 and ref_inode != parent[parent_inode]):
            print('DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + file_name + ' LINK TO INODE ' + str(ref_inode) + ' SHOULD BE ' + str(parent[parent_inode]))
            status = 1

def unreferenced_block(block):
    global block_state, status
    if(is_legal_block(block)):
        s = block_state.get(block)
        if(s != 'FREE' and s != 'IN USE' and s != 'DUPLICATE'):
            print('UNREFERENCED BLOCK ' + str(block))
            status = 1

def main():
    #Global initialize variables
    global inodes, dir, indir, free_inodes, free_blocks
    global inode_state, block_state, links, parent, num_blocks
    global first_nonreserved_ino, num_inodes, in_use, status

    # Check for invalid argument count
    if (len(sys.argv) == 2):
        try:
            # Read inputted csv file
            f = open(sys.argv[1],'r')
            contents = f.read()
            f.close()
        except IOError:
            sys.stderr.write('I/O error: Unable to open file\n')
            exit(1)
    else:
        sys.stderr.write('Argument error: Invalid argument count\n')
        exit(1)

    # Parse the csv file into lists
    lines = contents.split('\n')
    details = []
    for i in lines:
        temp_details = i.split(',')
        details.append(temp_details)

    if details[0][0] == 'SUPERBLOCK':
        num_blocks = int(details[0][1])
        num_inodes = int(details[0][2])
        first_nonreserved_ino = int(details[0][7])

    # Create a list for free blocks
    for i in range(1, num_blocks + 1):
        free_blocks.append(0)
        in_use[i - 1] = 0

    # Create a list for free inodes
    for i in range(1, num_inodes + 1):
        free_inodes.append(0)

    for i in details:
        s = i[0]
        if s == 'BFREE':
            bnum = int(i[1])
            free_blocks[bnum] = 1
        elif s == 'IFREE':
            inum = int(i[1])
            free_inodes[inum - 1] = 1
        elif s == 'INODE':
            # Refer to the inode's details through inum
            key = int(i[1])
            value = i
            inodes[key] = value
        elif s == 'DIRENT':
            dir.append(i)
        elif s == 'INDIRECT':
            indir.append(i)

    for i in range(0, num_blocks):
        if (not is_legal_block(i)):
            block_state[i] = 'RESERVED'
        elif (is_free_block(i)):
            block_state[i] = 'FREE'

    for i in range(1, num_inodes + 1):
        temp = inodes.get(i)
        links.append(0)
        parent.append(0)
        if(temp != None):
            if(temp[2] == 0):
                inode_state[i] = 0;
            else:
                inode_state[i] = int(temp[6])
        else:
            inode_state[i] = 0

    # Set the parent of the inode equal to itself
    EXT2_ROOT_INO = 2
    parent[0] = EXT2_ROOT_INO

    for i in range(1, num_inodes + 1):
        scan_inodes(i)

    for i in indir:
        scan_indir(i)

    for i in range(first_nonreserved_ino, num_blocks):
        unreferenced_block(i)

    for i in dir:
        scan_dir(i)

    for i in range(1, num_inodes + 1):
        if(int(links[i - 1]) != inode_state.get(i)):
            print('INODE ' + str(i) + " HAS " + str(links[i-1]) + ' LINKS BUT LINKCOUNT IS ' + str(inode_state.get(i)))
            status = 1

    for i in dir:
        scan_dir_2(i)

    if(status == 1):
        exit(2)
    exit(0)

if __name__ == '__main__':
    main()
