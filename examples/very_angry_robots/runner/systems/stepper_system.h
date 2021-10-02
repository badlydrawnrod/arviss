#pragma once

void UpdateStepperSystem(void);

static struct
{
    void (*Update)(void);
} StepperSystem = {.Update = UpdateStepperSystem};
