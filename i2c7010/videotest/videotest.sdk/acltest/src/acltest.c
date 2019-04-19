#include "acl.h"
#define QUARTER 0.75
#define SFERT 0.15

void Run(int fd) {
	float x = 0.0f, y = 0.0f, z = 0.0f;

	printf("ACL Demo Launched\n");

	float average_accl_x = 0.0f, average_accl_y = 0.0f, average_accl_z = 0.0f;
	float accl_x = 0.0f, accl_y = 0.0f;


	//while (1) {
	for(int i=0;i<15;i++) {
		average_accl_x = 0.0f;
		average_accl_y = 0.0f;
		average_accl_z = 0.0f;
		for (int i = 0; i < 100; i++) {
			ACL_ReadAccelG(fd, &x, &y, &z);
			//printf("RawValues(%.2f,%.2f)\n", x, y);
			usleep(1000);
			average_accl_x += x;
			average_accl_y += y;
			average_accl_z += z;
		}

		average_accl_x /= 100.0f;
		average_accl_y /= 100.0f;
		average_accl_z /= 100.0f;

		//printf("RawValues(%.2f,%.2f)\n", average_accl_x, average_accl_y);

		//Calculating acceleration
		accl_x = average_accl_x * (float) 9.8f;  // m/s^2
		accl_y = average_accl_y * (float) 9.8f; // m/s^2

	}
	printf("Done!\n");
}

void Initialize(int fd) {
	ACL_SetMeasure(fd, 0);
	ACL_SetGRange(fd, ACL_PAR_GRANGE_PM4G);
	ACL_SetMeasure(fd, 1);
	ACL_CalibrateOneAxisGravitational(fd, ACL_PAR_AXIS_ZP);
	sleep(1); // After calibration, some delay is required for the new settings
			  // to take effect.
	printf("Initialize worked\n");
}


int main()
{
	int fd = init();
	printf("Let's start\n");
	Initialize(fd);
	Run(fd);

	return 0;
}

