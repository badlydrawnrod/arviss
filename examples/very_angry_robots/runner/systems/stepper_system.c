#include "stepper_system.h"

#include "entities.h"
#include "tables/steps.h"

void UpdateStepperSystem(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmStepped))
        {
            Step* s = Steps.Get(id);
            s->step = (s->step > 0) ? s->step - 1 : (s->rate - 1);
        }
    }
}
