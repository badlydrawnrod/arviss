#include "entities.h"

void UpdateRobotActions(void);

static struct
{
    void (*Update)(void);
} RobotActionSystem = {.Update = UpdateRobotActions};
