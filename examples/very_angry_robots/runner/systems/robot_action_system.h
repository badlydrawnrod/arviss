#include "entities.h"

void ResetRobotActions(void);
void UpdateRobotActions(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} RobotActionSystem = {.Reset = ResetRobotActions, .Update = UpdateRobotActions};
