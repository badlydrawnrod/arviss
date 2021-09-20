#include "controller.h"

static ControllerType controllerType = ctKEYBOARD;

void SetController(ControllerType newControllerType)
{
    controllerType = newControllerType;
}

ControllerType GetController(void)
{
    return controllerType;
}
