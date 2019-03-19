//
// Created by Croff on 2017/5/30.
//

#include "call.h"
#include "bottom.h"

#define PAGE_SIZE (4 * 1024)
#define PAGE_TABLE_START 0
#define PAGE_TABLE_SIZE (896 * 1024)
#define V_PAGE_BIT_TABLE_START (896 * 1024)
#define V_PAGE_BIT_TABLE_SIZE (16 * 1024)
#define P_PAGE_BIT_TABLE_START (912 * 1024)
#define P_PAGE_BIT_TABLE_SIZE (4 * 1024 - 32)
#define SWAP_START (916 * 1024)
#define MEM_USE_RECORD_START (924 * 1024)
#define MEM_USE_RECORD_SIZE (100 * 1024)
#define MEM_DATA_START (1024 * 1024)
#define MEM_DATA_SIZE (1023 * 1024)

/*
 * 页表项
 */
struct page_table_item {
    unsigned int v_page_id; // 逻辑页面号
    unsigned int p_page_id; // 对应物理页框号
    char in;    // 是否在内存中
    char use;   // 最近是否使用
};

/*
 * 内存表：内存使用记录
 */
struct mem_use {
    m_pid_t pid;    // 进程号
    v_address address;  // 起始地址
    m_size_t size;  // 占用空间大小
};

/**
 * 根据在物理内存中存放的地址读取页表项
 *
 * @param address 在物理内存中存放的地址
 * @param page_item 页表项指针
 */
void read_page_table_item(p_address address, struct page_table_item *page_item) {
    data_unit data[7];
    int i;
    for (i = 0; i < 7; ++i) {
        data[i] = mem_read(address + i);
    }
    page_item->v_page_id = (data[0] << 16) + (data[1] << 8) + data[2];
    page_item->p_page_id = (data[3] << 16) + (data[4] << 8) + data[5];
    if (data[6] & 128) {
        page_item->in = 1;
    } else {
        page_item->in = 0;
    }
    if (data[6] & 64) {
        page_item->use = 1;
    } else {
        page_item->use = 0;
    }
}

/**
 * 在物理内存中的地址写入页表项
 *
 * @param address 在物理内存中的地址
 * @param page_item 页表项指针
 */
void write_page_table_item(p_address address, struct page_table_item *page_item) {
    data_unit data[7];
    data[0] = (data_unit) ((page_item->v_page_id >> 16) % 256);
    data[1] = (data_unit) ((page_item->v_page_id >> 8) % 256);
    data[2] = (data_unit) ((page_item->v_page_id) % 256);
    data[3] = (data_unit) ((page_item->p_page_id >> 16) % 256);
    data[4] = (data_unit) ((page_item->p_page_id >> 8) % 256);
    data[5] = (data_unit) ((page_item->p_page_id) % 256);
    data[6] = (data_unit) ((page_item->in << 7) + (page_item->use << 6));

    int i;
    for (i = 0; i < 7; ++i) {
        mem_write(data[i], address + i);
    }
}

/**
 * 根据逻辑页面号读取页表项
 *
 * @param v_page_id 逻辑页面号
 * @param page_item 页表项指针
 * @return 读取结果: 1为成功, -1为失败
 */
int find_page_table_item(unsigned int v_page_id, struct page_table_item *page_item) {
    if (v_page_id * PAGE_SIZE >= DISK_SIZE) {
        return -1;
    }

    p_address address = PAGE_TABLE_START + v_page_id * 7;
    read_page_table_item(address, page_item);
    if (page_item->v_page_id == v_page_id) {
        return 1;
    } else {
        return -1;
    }
}

/**
 * 根据逻辑页面号更新页表项
 *
 * @param v_page_id 逻辑页面号
 * @param page_item 页表项指针
 * @return 更新结果: 1为成功, -1为失败
 */
int update_page_table_item(unsigned int v_page_id, struct page_table_item *page_item) {
    if (v_page_id * PAGE_SIZE >= DISK_SIZE) {
        return -1;
    }

    p_address address = PAGE_TABLE_START + v_page_id * 7;
    write_page_table_item(address, page_item);
    if (page_item->v_page_id == v_page_id) {
        return 1;
    } else {
        return -1;
    }
}

/**
 * 根据物理页框号写入页框占用情况
 *
 * @param page_id 物理页框号
 * @param use 占用情况，1为占用，0为未占用
 * @return 写入结果: 1为成功, -1为失败
 */
int set_p_page_bit_table_value(unsigned int page_id, int use) {
    if (page_id * PAGE_SIZE >= MEM_DATA_SIZE) {
        return -1;
    }

    p_address base = P_PAGE_BIT_TABLE_START + page_id / 8;
    p_address offset = page_id % 8;
    data_unit data = mem_read(base);
    int origin = (data >> (7 - offset)) % 2;
    if (use != origin) {
        if (use) {
            data += (1 << (7 - offset));
        } else {
            data -= (1 << (7 - offset));
        }
        mem_write(data, base);
    }
    return 1;
}

/**
 * 根据在物理内存中存放的地址读取内存表项
 *
 * @param address 在物理内存中存放的地址
 * @param use 内存表项指针
 */
void read_mem_use(p_address address, struct mem_use *use) {
    data_unit data[10];
    int i;
    for (i = 0; i < 10; i++) {
        data[i] = mem_read(address + i);
    }

    use->pid = (data[0] << 8) + data[1];
    use->address = (data[2] << 24) + (data[3] << 16) + (data[4] << 8) + data[5];
    use->size = (data[6] << 24) + (data[7] << 16) + (data[8] << 8) + data[9];
}

/**
 * 在物理内存中的地址写入内存表项
 *
 * @param address 在物理内存中存放的地址
 * @param use 内存表项指针
 */
void write_mem_use(p_address address, struct mem_use *use) {
    data_unit data[10];
    data[0] = (data_unit) ((use->pid >> 8) % 256);
    data[1] = (data_unit) (use->pid % 256);
    data[2] = (data_unit) ((use->address >> 24) % 256);
    data[3] = (data_unit) ((use->address >> 16) % 256);
    data[4] = (data_unit) ((use->address >> 8) % 256);
    data[5] = (data_unit) (use->address % 256);
    data[6] = (data_unit) ((use->size >> 24) % 256);
    data[7] = (data_unit) ((use->size >> 16) % 256);
    data[8] = (data_unit) ((use->size >> 8) % 256);
    data[9] = (data_unit) (use->size % 256);

    int i;
    for (i = 0; i < 10; i++) {
        mem_write(data[i], address + i);
    }
}

/**
 * 物理页框对换，交换数据区，更新页表项
 *
 * @param swap_in 换进内存的物理页框
 * @param swap_out 换出内存的物理页框
 */
void swap_p_page(struct page_table_item *swap_in, struct page_table_item *swap_out) {
    p_address mem_address = MEM_DATA_START + swap_out->p_page_id * PAGE_SIZE;
    p_address disk_address = swap_in->v_page_id * PAGE_SIZE;
    p_address offset;

    //将要换出内存的数据写入交换区
    for (offset = 0; offset < PAGE_SIZE; ++offset) {
        data_unit data = (data_unit) mem_read(mem_address + offset);
        mem_write(data, SWAP_START + offset);
    }

    //将磁盘中的数据写入内存
    disk_load(mem_address, disk_address, PAGE_SIZE);

    //将交换区的数据写入磁盘
    disk_save(SWAP_START, disk_address, PAGE_SIZE);

    //更新页表项并保存
    unsigned int page_id = swap_in->p_page_id;
    swap_in->use = 1;
    swap_in->in = 1;
    swap_in->p_page_id = swap_out->p_page_id;
    update_page_table_item(swap_in->v_page_id, swap_in);
    swap_out->use = 0;
    swap_out->in = 0;
    swap_out->p_page_id = page_id;
    update_page_table_item(swap_out->v_page_id, swap_out);
}

void init() {
    p_address address;

    // 初始化内存数据区
    for (address = MEM_DATA_START; address < MEMORY_SIZE; address++) {
        mem_write(0, address);
    }

    // 初始化磁盘
    for (address = 0; address < DISK_SIZE; address += (MEMORY_SIZE / 2)) {
        disk_save(MEMORY_SIZE / 2, address, MEMORY_SIZE / 2);
    }

    // 初始化页表
    for (address = PAGE_TABLE_START; address < PAGE_TABLE_SIZE; address += 7) {
        struct page_table_item item;
        unsigned int page_id = address / 7;
        item.v_page_id = page_id;
        item.p_page_id = (1 << 24) - 1;
        item.in = 0;
        item.use = 0;
        write_page_table_item(address, &item);
    }

    // 初始化位示图
    for (address = V_PAGE_BIT_TABLE_START;
         address < V_PAGE_BIT_TABLE_START + V_PAGE_BIT_TABLE_SIZE + P_PAGE_BIT_TABLE_SIZE; address++) {
        data_unit data = 0;
        mem_write(data, address);
    }

    // 初始化内存表
    struct mem_use use;
    use.pid = 1000;
    for (address = MEM_USE_RECORD_START; address < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE; address += 10) {
        write_mem_use(address, &use);
    }
}

int read(data_unit *data, v_address address, m_pid_t pid) {
    if (address >= DISK_SIZE) {
        return -1;
    }

    int found = 0;

    // 查找内存表，找出对应记录
    p_address p_add = MEM_USE_RECORD_START;
    struct mem_use use;
    while (p_add < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE) {
        read_mem_use(p_add, &use);
        p_add += 10;
        if (use.pid < 1000) {
            if (use.pid == pid && address >= use.address && address < use.address + use.size) {
                // 找到pid和v_address对应的记录
                found = 1;
                break;
            } else {
                continue;
            }
        } else {
            break;
        }
    }

    if (!found) {
        // 内存表中未找到相关记录
        return -1;
    }

    // 根据逻辑地址计算出逻辑页号和偏移量
    unsigned int v_page_id = address / PAGE_SIZE;
    unsigned int page_offset = address % PAGE_SIZE;

    // 根据逻辑页号查页表获得对应的物理页框号
    unsigned int p_page_id = 0;
    struct page_table_item item;
    find_page_table_item(v_page_id, &item);
    if (!item.in) {
        // 对应页框不在内存中
        unsigned int scan_page_id = v_page_id / 4;
        struct page_table_item scan_item;
        while (1) {
            find_page_table_item(scan_page_id, &scan_item);
            if (scan_item.in) {
                if (scan_item.use) {
                    scan_item.use = 0;
                    update_page_table_item(scan_page_id, &scan_item);
                } else {
                    swap_p_page(&item, &scan_item);
                    break;
                }
            }

            scan_page_id--;
            if (scan_page_id < 0) {
                scan_page_id = PAGE_TABLE_SIZE / 7 - 1;
            }
        }
    } else {
        item.use = 1;
        update_page_table_item(v_page_id, &item);
    }
    p_page_id = item.p_page_id;

    // 根据物理页框号和偏移量，读取需要的数据
    p_add = MEM_DATA_START + p_page_id * PAGE_SIZE + page_offset;
    *data = mem_read(p_add);
    return 0;
}

int write(data_unit data, v_address address, m_pid_t pid) {
    if (address >= DISK_SIZE) {
        return -1;
    }

    int found = 0;

    // 查找内存表，找出对应记录
    p_address p_add = MEM_USE_RECORD_START;
    struct mem_use use;
    while (p_add < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE) {
        read_mem_use(p_add, &use);
        p_add += 10;
        if (use.pid < 1000) {
            if (use.pid == pid && address >= use.address && address < use.address + use.size) {
                // 找到pid和v_address对应的记录
                found = 1;
                break;
            } else {
                continue;
            }
        } else {
            break;
        }
    }

    if (!found) {
        // 内存表中未找到相关记录
        return -1;
    }

    // 根据逻辑地址计算出逻辑页号和偏移量
    unsigned int v_page_id = address / PAGE_SIZE;
    unsigned int page_offset = address % PAGE_SIZE;

    // 根据逻辑页号查页表获得对应的物理页框号
    unsigned int p_page_id = 0;
    struct page_table_item item;
    find_page_table_item(v_page_id, &item);
    if (!item.in) {
        // 对应页框不在内存中
        unsigned int scan_page_id = v_page_id / 4;
        struct page_table_item scan_item;
        while (1) {
            find_page_table_item(scan_page_id, &scan_item);
            if (scan_item.in) {
                if (scan_item.use) {
                    scan_item.use = 0;
                    update_page_table_item(scan_page_id, &scan_item);
                } else {
                    swap_p_page(&item, &scan_item);
                    break;
                }
            }

            scan_page_id--;
            if (scan_page_id < 0) {
                scan_page_id = PAGE_TABLE_SIZE / 7 - 1;
            }
        }
    } else {
        item.use = 1;
        update_page_table_item(v_page_id, &item);
    }
    p_page_id = item.p_page_id;

    // 根据物理页框号和偏移量，写入需要的数据
    p_add = MEM_DATA_START + p_page_id * PAGE_SIZE + page_offset;
    mem_write(data, p_add);
    return 0;
}

int allocate(v_address *address, m_size_t size, m_pid_t pid) {
    if (size > DISK_SIZE) {
        return -1;
    }

    // 计算需要的页数
    int req_page_num = size / PAGE_SIZE;
    if (size > req_page_num * PAGE_SIZE) {
        req_page_num++;
    }

    // 找到有足够页数的连续逻辑内存空间，记录起始页号和页数量
    unsigned int found_start_page_id = 0;
    unsigned int found_page_num = 0;
    p_address p_add = V_PAGE_BIT_TABLE_START;
    do {
        data_unit data = mem_read(p_add);
        p_add++;
        int i;
        for (i = 0; i < 8; i++) {
            found_page_num++;
            if ((data >> (7 - i)) % 2) {
                found_start_page_id += found_page_num;
                found_page_num = 0;
            } else {
                if (found_page_num == req_page_num) {
                    break;
                }
            }
        }
    } while (found_page_num < req_page_num && p_add < V_PAGE_BIT_TABLE_START + V_PAGE_BIT_TABLE_SIZE);

    // 没有足够的连续空间以分配
    if (found_page_num < req_page_num) {
        return -1;
    }

    // 计算起始地址
    *address = found_start_page_id * PAGE_SIZE;

    // 更新逻辑内存页位示图
    unsigned int update_page_id = found_start_page_id;
    p_add = V_PAGE_BIT_TABLE_START + (found_start_page_id / 8);
    while (update_page_id < found_start_page_id + found_page_num) {
        data_unit data = mem_read(p_add);
        int i;
        for (i = update_page_id % 8; i < 8; i++) {
            data += (1 << (7 - i));
            update_page_id++;
            if (update_page_id == found_start_page_id + found_page_num) {
                break;
            }
        }

        mem_write(data, p_add);
        p_add++;
    }

    // 将每页逻辑内存分配到可用的物理内存页中，更新物理存储页位示图和页表
    p_add = P_PAGE_BIT_TABLE_START;
    unsigned int v_page_id = found_start_page_id;
    unsigned int p_page_id = 0;
    while (found_page_num && p_add < P_PAGE_BIT_TABLE_START + P_PAGE_BIT_TABLE_SIZE) {
        data_unit data = mem_read(p_add);
        int i;
        for (i = 0; i < 8; i++) {
            if ((data >> (7 - i)) % 2 == 0) {
                //更新位示图和页表
                data += (1 << (7 - i));
                struct page_table_item item;
                if (find_page_table_item(v_page_id, &item)) {
                    item.p_page_id = p_page_id;
                    item.use = 0;
                    item.in = 1;
                    update_page_table_item(v_page_id, &item);
                    v_page_id++;
                    found_page_num--;
                    if (found_page_num == 0) {
                        break;
                    }
                }
            }
            p_page_id++;
        }

        mem_write(data, p_add);
        p_add++;
    }

    // 添加内存表记录
    p_add = MEM_USE_RECORD_START;
    struct mem_use use;
    while (p_add < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE) {
        read_mem_use(p_add, &use);
        if (use.pid >= 1000) {
            use.pid = pid;
            use.address = *address;
            use.size = size;
            write_mem_use(p_add, &use);
            break;
        }
        p_add += 10;
    }

    return 0;
}

int free(v_address address, m_pid_t pid) {
    if (address >= DISK_SIZE) {
        return -1;
    }

    int found = 0;

    // 查找内存表，找出对应记录
    p_address p_add = MEM_USE_RECORD_START;
    struct mem_use use;
    while (p_add < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE) {
        read_mem_use(p_add, &use);
        p_add += 10;
        if (use.pid < 1000) {
            if (use.pid == pid && use.address == address) {
                // 找到pid和v_address对应的记录
                found = 1;
                break;
            }
        } else {
            // 内存表记录区域结束
            break;
        }
    }

    if (!found) {
        // 内存表中未找到相关记录
        return -1;
    }

    unsigned int start_page_id = use.address / PAGE_SIZE;
    unsigned int use_page_number = use.size / PAGE_SIZE;
    if (use.size > use_page_number * PAGE_SIZE) {
        use_page_number++;
    }

    // 删除这条记录，把后面的记录向前移动一个单位
    while (p_add < MEM_USE_RECORD_START + MEM_USE_RECORD_SIZE && use.pid < 1000) {
        read_mem_use(p_add, &use);
        write_mem_use(p_add - 10, &use);
        p_add += 10;
    }
    use.pid = 1000;
    write_mem_use(p_add - 10, &use);

    // 更新逻辑内存位示图，释放占用
    unsigned int page_id = start_page_id;
    struct page_table_item item;
    p_add = V_PAGE_BIT_TABLE_START + start_page_id / 8;
    while (page_id < start_page_id + use_page_number && p_add < V_PAGE_BIT_TABLE_START + V_PAGE_BIT_TABLE_SIZE) {
        data_unit data = mem_read(p_add);
        int i;
        for (i = page_id % 8; i < 8; i++) {
            data -= (1 << (7 - i));

            // 更新物理存储位示图，释放占用
            find_page_table_item(page_id, &item);
            unsigned int p_page_id = item.p_page_id;
            if (p_page_id < MEM_DATA_SIZE / PAGE_SIZE) {
                set_p_page_bit_table_value(p_page_id, 0);
            }

            page_id++;
            if (page_id == start_page_id + use_page_number) {
                break;
            }
        }

        mem_write(data, p_add);
        p_add++;
    }

    return 0;
}
