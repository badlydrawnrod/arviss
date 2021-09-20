#pragma once

typedef enum ControllerType
{
    ctKEYBOARD,
    ctGAMEPAD0,
} ControllerType;

void SetController(ControllerType controllerType);
ControllerType GetController(void);
