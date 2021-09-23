#include "entities.h"

void ResetPlayerControllers(void);
void UpdatePlayerControllers(void);
void HandleTriggersPlayerControllers(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
    void (*HandleTriggers)(void);
} PlayerControllerSystem = {.Reset = ResetPlayerControllers,
                            .Update = UpdatePlayerControllers,
                            .HandleTriggers = HandleTriggersPlayerControllers};
