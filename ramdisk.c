/*=====================================================================================*/
// A request handling which behaves as a ramdisk
//
// This part is not used by the project, but can be in case
// I need to compare how is Linux behaving

char *data = NULL;

static void ramdisk_transfer(sector_t sector,
		unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * 512;
	unsigned long nbytes = nsect * 512;

	if ((offset + nbytes) > nb_sectors * 512) {
		printk (KERN_NOTICE "SaWa: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
    
    printk(KERN_INFO "SaWa: Request for %lu sectors starting at sector #%lu (write=%d)\n", nsect, sector, write);
    
	if (write) {
        dump_mem(buffer, 32);
		memcpy(data + offset, buffer, nbytes);
    }
	else
		memcpy(buffer, data + offset, nbytes);
}

static void ramdisk_request(struct request_queue *q) {
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		// blk_fs_request() was removed in 2.6.36 - many thanks to
		// Christian Paro for the heads up and fix...
		//if (!blk_fs_request(req)) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}
		ramdisk_transfer(blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
        else {
//            printk(KERN_INFO "Same request?\n");
        }
//        printk(KERN_INFO "Request ended. New request = %px\n", req);
	}
}
