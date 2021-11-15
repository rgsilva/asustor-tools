#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_CURVE 255
#define MAX_FANS 10
#define ARG_DELIM ","
#define TEMP_DELIM ':'
#define MAX_DISKS 10
#define MAX_DISK_NAME 8
#define CHECK_TEMP_INTERVAL 10

// Fan functions

extern int Hal_Fan_Get_Raw_Value(ulong p1, uint *p2);
extern int Hal_Fan_Set_Raw_Value(ulong p1, uint p2);

// System and CPU thermal functions

#define SYSTEMP 2
#define CPUTEMP 3

extern int _Z29Hal_Sensor_Get_Thermal_Status18HAL_EN_SENSOR_TYPERi(int p1, int *p2);
#define Hal_Sensor_Get_Thermal_Status _Z29Hal_Sensor_Get_Thermal_Status18HAL_EN_SENSOR_TYPERi

// HDD functions

extern int Hal_Disk_Get_Temperature(char *p1, double *p2, char *p3);
extern int _Z20Get_Disk_Device_NameiiPc(int p1, int p2, char *p3);

// Our magic

typedef struct {
  int temp;
  int pwm;
} CurveItem;
CurveItem EmptyCurveItem = {-1, -1};

CurveItem curve[MAX_CURVE];
int fans[MAX_FANS];
uint sleepTime;
char disks[MAX_DISK_NAME][MAX_DISKS];
uint checkTemp;

static volatile int keepRunning = 1;

void signalHandler(int _) {
  keepRunning = 0;
}

void help(char *arg0) {
    printf("%s <fans> <sleep> <curve>\n", arg0);
    printf("\n");
    printf("fans:  comma-separated fan indexes\n");
    printf("  ex:  0,1,2,3\n");
    printf("sleep: nanoseconds to sleep between each PWM check\n");
    printf("  ex:  10000\n");
    printf("curve: comma-separated temp:pwm ordered by temp\n");
    printf("  ex:  30:0,40:50,50:100,60:150,70:255\n");
}

void parseFansArg(char *arg) {
  for (int i = 0; i < MAX_FANS; i++) { fans[i] = -1; }

  int index = 0;
  char *ptr = strtok(arg, ARG_DELIM);
  while (ptr != NULL) {
    if (index < MAX_FANS) {
      fans[index] = atoi(ptr);
    } else {
      printf("WARN: ignoring fan %s: too many fans!\n", ptr);
    }
    ptr = strtok(NULL, ARG_DELIM);
    index++;
  }

  printf("Fans: ");
  for (int i = 0; i < MAX_FANS; i++) {
    if (fans[i] != -1) {
      printf("%d ", fans[i]);
    }
  }
  printf("\n");
}

void parseSleepArg(char *arg) {
  sleepTime = atoi(arg);
  printf("Sleep: %u ns\n", sleepTime);
}

CurveItem parseCurveItem(char *arg) {
  char temp[32] = {0};
  char pwm[32] = {0};

  char *pwmPointer = strchr(arg, TEMP_DELIM);
  strncpy(pwm, pwmPointer + sizeof(char), sizeof(pwm));

  pwmPointer = 0;
  strncpy(temp, arg, sizeof(temp));

  CurveItem result = { atoi(temp), atoi(pwm) };
  return result;
}

void parseCurveArg(char *arg) {
  for (int i = 0; i < MAX_CURVE; i++) { curve[i] = EmptyCurveItem; }

  int index = 0;
  char *ptr = strtok(arg, ARG_DELIM);
  while (ptr != NULL) {
    if (index < MAX_CURVE) {
      CurveItem item = parseCurveItem(ptr);
      curve[index] = item;
    } else {
      printf("WARN: ignoring curve item %s: too many items!\n", ptr);
    }
    ptr = strtok(NULL, ARG_DELIM);
    index++;
  }

  printf("Curve: ");
  for (int i = 0; i < MAX_CURVE; i++) {
    printf("%d -> %d", curve[i].temp, curve[i].pwm);
    if (i < MAX_CURVE-1 && curve[i+1].temp != -1) {
      printf(", ");
    } else {
      break;
    }
  }
  printf("\n");
}

void countDisks() {
  char buffer[32] = {0};
  int retval = 0;
  int i;

  for (i = 0; i < MAX_DISKS; i++) { disks[i][0] = 0; }

  for (i = 0; i < MAX_DISKS; i++) {
    retval = _Z20Get_Disk_Device_NameiiPc(i, 0x10, buffer);
    if (retval == 0) {
      break;
    }

    double t = 0;
    retval = Hal_Disk_Get_Temperature(buffer, &t, NULL);
    if (t == 0.00 || retval != 0) {
      break;
    }

    strncpy(disks[i], buffer, MAX_DISK_NAME);
  }

  printf("Found %d disks: ", i);
  int diskCount = i;
  for (i = 0; i < diskCount; i++) {
    printf("%s ", disks[i]);
  }
  printf("\n");
}

int averageTemp() {
  int retval;
  int temps[MAX_DISKS + 2] = {0};

  Hal_Sensor_Get_Thermal_Status(SYSTEMP, &temps[0]);
  Hal_Sensor_Get_Thermal_Status(CPUTEMP, &temps[1]);

  for (int i = 0; i < MAX_DISKS; i++) {
    if (disks[i][0] == 0) { break; }

    double t = 0;
    Hal_Disk_Get_Temperature(disks[i], &t, NULL);
    temps[i + 2] = (int)t;
  }

  int sum = 0;
  int count = 0;
  for (count = 0; count < MAX_DISKS + 2; count++) {
    if (temps[count] != 0) {
      sum += temps[count];
    } else {
      break;
    }
  }
  return ((double)sum)/count;
}

int tempToPwm(int temp) {
  if (temp < curve[0].temp) {
    return curve[0].pwm;
  }

  for (int i = 0; i < MAX_CURVE-1; i++) {
    if (curve[i].temp <= temp && (
      curve[i+1].temp >= temp || curve[i+1].temp == -1
    )) {
      return curve[i].pwm;
    }
  }

  return -1;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    help(argv[0]);
    return 1;
  }

  parseFansArg(argv[1]);
  parseSleepArg(argv[2]);
  parseCurveArg(argv[3]);
  countDisks();

  signal(SIGINT, signalHandler);
  signal(SIGQUIT, signalHandler);
  signal(SIGTERM, signalHandler);

  int fan_pwm = 0 & 0xFF;

  while (keepRunning) {
    int fansPwm[MAX_FANS] = {-1};

    // Check the current average system temperature.
    if (checkTemp == 0 || checkTemp == CHECK_TEMP_INTERVAL) {
      int avgTemp = averageTemp();
      printf("Average system temperature: %d\n", avgTemp);
      checkTemp = 1;
      fan_pwm = tempToPwm(avgTemp);
      if (fan_pwm == -1) {
        printf("WARN: unknown point in the curve! This should not happen! Using the first one to avoid fire!\n");
        fan_pwm = curve[0].pwm;
      }
    }

    // Check if we must update the fan speeds.
    int mustUpdate = 0;
    for (int i = 0; i < MAX_FANS; i++) {
      if (fans[i] == -1) { continue; }

      uint current_pwm = 0;
      int retval = Hal_Fan_Get_Raw_Value(fans[i], &current_pwm);
      if (retval != 0) {
        printf("FAILED: return value is %d\n", retval);
      }
      mustUpdate |= (current_pwm != fan_pwm);
    }

    // Update if we must.
    if (mustUpdate) {
      for (int i = 0; i < MAX_FANS; i++) {
        if (fans[i] == -1) { break; }

        int fan_index = fans[i];
        printf("Setting fan %d PWM value to %u\n", fan_index, fan_pwm);

        int retval = Hal_Fan_Set_Raw_Value(fan_index, fan_pwm);
        if (retval != 0) {
          printf("FAILED: return value is %d\n", retval);
          continue;
        }
      }
    }

    // Sleep
    usleep(sleepTime);
  }
  printf("Process exited gracefully\n");

  return 0;
}
