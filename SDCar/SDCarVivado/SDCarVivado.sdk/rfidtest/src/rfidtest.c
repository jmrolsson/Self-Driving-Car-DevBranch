#include "PN532_rfid.h"

#define TIMEOUT 1000

struct card {
	uint8_t type;
	uint32_t UID;
};

void printcard(struct card card){
	printf("Card UID: %d, type: %d\n", card.UID, card.type);
}

int main() {
	int fd = init();
	if(!fd) {
		printf("Error opening device");
		return -1;
	}
	uint32_t versiondata = getFirmwareVersion(fd);
	if (! versiondata) {
		printf("Didn't find PN53x board\n");
		return -1; // halt
	}

	printf("Found chip PN5"); printf("%x\n", (versiondata>>24) & 0xFF);
	printf("Firmware ver. "); printf("%d", (versiondata>>16) & 0xFF);
	printf("."); printf("%d\n", (versiondata>>8) & 0xFF);

	SAMConfig(fd);

	printf("Configured SAM\n");

	uint8_t success;
	uint8_t uid[6];
	uint8_t uidLength;

	uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; //default key
	uint8_t block = 4;

	struct card lastcard;

	sleep(1);
	printf("You can now use cards\n");

	while (1){
		printf("Waiting for card\n");
		memset(uid, 0, 6);
		success = readPassiveTargetID(fd, PN532_MIFARE_ISO14443A, uid, &uidLength, 0);
		if(success) {
			printf("Found an ISO14443A card\n\tUID Length: %d bytes\n\tUID Value: ", uidLength);
			if(uidLength != 4)
				continue;

			success = mifareclassic_AuthenticateBlock(fd, uid, uidLength, block, 0, keya);
			if (success) {
				uint8_t data[16];

				success = mifareclassic_ReadDataBlock(fd, block, data);
				if (success) {
					lastcard.type=data[0];
					memcpy(&lastcard.UID, uid, 4);
					printcard(lastcard);
				}
				else
					printf("Read block failed.\n");
			} else
				printf("Block auth failed.\n");
		}
	}
	close(fd);
}

