/ (type d) (inode block 0)
	owner:
	size: 1
	data disk blocks: 2
	entry 0: dir, inode block 1
	entry 1: new, inode block 5
	entry 2: file, inode block 10

/dir (type d) (inode block 1)
	owner: user1
	size: 1
	data disk blocks: 4
	entry 0: file, inode block 3

/dir/file (type f) (inode block 3)
	owner: user1
	size: 0
	data disk blocks:


/new (type d) (inode block 5)
	owner: user1
	size: 1
	data disk blocks: 7
	entry 0: dir, inode block 6

/new/dir (type d) (inode block 6)
	owner: user1
	size: 1
	data disk blocks: 9
	entry 0: file, inode block 8

/new/dir/file (type f) (inode block 8)
	owner: user1
	size: 0
	data disk blocks:


/file (type f) (inode block 10)
	owner: user1
	size: 0
	data disk blocks:


4085 disk blocks free
