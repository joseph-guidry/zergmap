#include "decode.h"
#include "splay.h"

#define NO_GPS 1000.00
#define NO_HEALTH 10
#define UDP_DEST 0x0ea7
#define FILE_LEN 60

struct zerg * create_zerg(unsigned int dest_id);
void destroy_zerg(struct zerg * old_zerg);


uint16_t get_root_srcID(void * data)
{
	return ((struct zerg*)data)->srcID;
} 


void display(const void * data)
{
	printf("display node\n");
	printf("zerg_id: %u \n", ((struct zerg*)data)->srcID );
	printf("zerg_id: %06f \n",  ((struct zerg*)data)->position.latitude.value );
	printf("zerg_id: %06f \n",  ((struct zerg*)data)->position.longitude.value );
	printf("zerg_id: %06f \n",  ((struct zerg*)data)->position.altitude.value );
	printf("\n");
}


struct node * decode (int argc, char **argv, int * node_count)
{
	FILE *fp;
	struct zergPacket pcapfile;
	int file, msgType, filesize, sum;
	char filename[FILE_LEN];
	if (argc < 2)
	{
		fprintf(stderr, "%s: usage error < input file >\n", argv[0]);
		exit(1);
	}
	
	struct node * root = NULL;
	
	//Iterate pcap build for a sequence of files from the command line
	for ( file = 1; file < argc; file++) 
	{
		//Fill pcap structure with individual header structures.
		strcpy(filename, argv[file]);
		fp = buildPcapData(&pcapfile, filename, &filesize);
		if (fp != NULL)
		{
		//Attempt to as many pcaps in a singe file.
			while((ftell(fp) < filesize) && ((filesize - ftell(fp)) > FILE_LEN))
			{
				
			
				fp = buildPacketData(&pcapfile, fp);
			
				msgType = ((htonl(pcapfile.pcapZerg.ver_type_totalLen) >> 24) & 0x0f);
				// Create the zerg_info structure
				//printf("going into zerg_info\n");
				struct zerg * zerg_info = create_zerg( htons(pcapfile.pcapZerg.sourceID) );
				//printf("here1\n");
				switch (msgType)
				{
					case 0:
						//printf("getting msg payload\n");
						fp = printMsgPayload(&pcapfile, fp);
						break;
					case 1:
						//printf("getting status data\n");
						fp = fillStatusPayload(&pcapfile, fp, zerg_info);
						break;
					case 2:
						//printf("getting cmd payload\n");
						fp = printCmdPayload(fp);
						break;
					case 3:
						//printf("getting gps data\n");
						fp = fillGpsPayload(fp, zerg_info);
						break;
					default:
						//printf("Unknown payload type\n");
						break;
				}
			
				if ( htons( pcapfile.pcapUdp.dport) == UDP_DEST ||  (htonl(pcapfile.pcapZerg.ver_type_totalLen) >> 28 == 1))
				{
					/* Build a zerg_info structure for the pcap that was read */
					
					root = insert(root, zerg_info->srcID, sizeof(struct zerg), get_root_srcID);
					if(   ! ((struct zerg*)root->key)->srcID )
					//if root = null -> new node then key == null
					{
						if( msgType == 1)
						{
							((struct zerg*)root->key)->number = (*node_count);
							((struct zerg*)root->key)->srcID = zerg_info->srcID;
							((struct zerg*)root->key)->health.hit_points = zerg_info->health.hit_points;
							((struct zerg*)root->key)->health.max_points = zerg_info->health.max_points;
							((struct zerg*)root->key)->position.latitude.value = NO_GPS;
							((struct zerg*)root->key)->position.longitude.value = NO_GPS;
							((struct zerg*)root->key)->position.altitude.value = NO_GPS;
							(*node_count)++;
						}
						//	update gps data
						else if ( msgType == 3)
						{
							((struct zerg*)root->key)->number = (*node_count);
							((struct zerg*)root->key)->srcID = zerg_info->srcID;
							((struct zerg*)root->key)->health.hit_points = NO_HEALTH;
							((struct zerg*)root->key)->health.max_points = NO_HEALTH;
							((struct zerg*)root->key)->position.latitude.value = zerg_info->position.latitude.value;
							((struct zerg*)root->key)->position.longitude.value = zerg_info->position.longitude.value;
							((struct zerg*)root->key)->position.altitude.value = zerg_info->position.altitude.value;
							(*node_count)++;
						}
					}
					else 
					{
						if( msgType == 1)
						{
							((struct zerg*)root->key)->health.hit_points = zerg_info->health.hit_points;
							((struct zerg*)root->key)->health.max_points = zerg_info->health.max_points;
						}
						//	update gps data
						else if ( msgType == 3)
						{
							((struct zerg*)root->key)->position.latitude.value = zerg_info->position.latitude.value;
							((struct zerg*)root->key)->position.longitude.value = zerg_info->position.longitude.value;
							((struct zerg*)root->key)->position.altitude.value = zerg_info->position.altitude.value;
						}
				
					}
				}
				/*
				//HANDLE PADDING OR BLANK FCS at end of the pcap.
				//FROM original decode.c implementation.
				fread(filename, 1, 4, fp);
				for (unsigned int x = 0; x < strlen(filename); x++)
				{
					sum += filename[x];
					if (sum != 0)
					{
						fseek(fp, -4, SEEK_CUR);
						break;
					}
				}
				*/
				
				//Free the malloc'd data in making a zerg info struct from pcap data.
				free(zerg_info) ;
			}
		}
		if (fp != NULL)
			fclose(fp);
	}
	
	//For test return 1 for success
	return root;
}

struct zerg * create_zerg(unsigned int src_id)
{
	struct zerg * new_zerg = malloc(sizeof(struct zerg));
	if ( new_zerg )
		new_zerg->srcID = src_id;
	return new_zerg;	
	
}

void destroy_zerg(struct zerg * old_zerg)
{
	if( old_zerg != NULL)
	{
		free(old_zerg->zerg_name);
		free(old_zerg);
	}
}



