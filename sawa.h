extern unsigned int nb_sectors;

int sawa_send_info(void);
int sawa_write_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer);
int sawa_read_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer_in);
