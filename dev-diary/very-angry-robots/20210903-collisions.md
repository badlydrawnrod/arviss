# Collisions and Events in Very Angry Robots III

**3/9/21**

In Very Angry Robots III, there are several types of entity. Some of these are mobile, such as the player, the robots, and their respective shots. Others are generally static, such as walls and doors.

## Collision Detection

One of the problems to solve is that of collision detection, by which I mean determining if two entities have hit each other in the game world. For the time being, testing if two entities collide is done by checking if their bounding rectangles overlap. It's simple, and it's good enough for now.

The bounding rectangle itself is determined by getting the entity's collision rectangle from a table indexed by a value held in its collision component, then offsetting that rectangle by the entity's position.

There's no doubt that this could be made faster by reducing the number of entities tested for collision, perhaps by spatial hashing or a [quad tree](https://en.wikipedia.org/wiki/Quadtree), but for now the number of entities is so astonishingly small and the cost of checking for a collision is so cheap that it probably isn't worth it.

## Collision Response

Here's where it gets more interesting. If we know that two entities have collided, what then? The answer to that largely depends on the entity.

For example, if the player is hit, then quite a number of things need to happen:

- The player should die
- A death animation should be played
- A particle effect explosion should be created at the player's position
- A sound effect should be played
- The player should lose a life
- The robots should stop firing at the player
- ...and so on.

Player death is a more extreme example than most, but it's clear that none of these things belong in the collision detection system. Should the collision detection system know about starting explosions, or playing sound effects? No, of course not. These are all things that happen in response to an event raised by the collision detection system - a **collision event**.

## Events

Before I go any further, I should spell out that an event is a way of describing something that has *already happened*. Events can be delivered by many mechanisms, such as messages, or callbacks, but they should not be conflated with their delivery mechanism. Other things might happen as a result of an event, but an event is never an instruction for those things to happen - it's simply a record of the fact that something *has* happened.

To illustrate, here's a table. Everything in the left column is an event - it's something that has already occurred. Everything in the right column might be an action that happens as a result of an event, but it is not an event itself.

| Event                      | Not an Event                                    |
| -------------------------- | ----------------------------------------------- |
| the player has died        | kill the player                                 |
| the game has ended         | play a death animation at the player's position |
| a robot has fired a bullet | play a sound effect                             |

## Representing Events

It took a while to decide how to represent events because I wasn't sure if I wanted an immediate response to them or not. But as I'd started down the route of implementing an [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system), I had started to think of components in terms of database tables, such as *Positions*, *Velocities*, and so on, in which each component was accessed by its entity id. It wasn't a huge leap forward to think of adding *Events* as another "table", but this time accessed by an event id rather than entity id ... and that broke me free from thinking that everything in an ECS must be about entities.[^1]

An event is raised by adding it to the *Events* table. Here's an example showing the creation of a collision event.

```c
if (CheckCollisionRecs(shotRect, otherRect))
{
    Events.Add(&(Event){.type = etCOLLISION,
                        .collision = (CollisionEvent){.firstId = shotId, .secondId = id}});
}
```

## Processing Events

Events are processed by the event system after the other systems have run. The event system iterates through each event in turn, and decides what to do with it based on its type. So far, it doesn't do much ... in fact, here it is in its entirety.

```c
for (int i = 0, numEvents = Events.Count(); i < numEvents; i++)
{
    const Event* e = Events.Get((EventId){.id = i});
    if (e->type == etCOLLISION)
    {
        const CollisionEvent* c = &e->collision;
        TraceLog(LOG_INFO, "Collision between %s and %s",
                 Identify(c->firstId),
                 Identify(c->secondId));

        // Tag the first entity for reaping, because it's always going to be the player,
        // a robot or a shot.
        Entities.Set(c->firstId, bmReap);

        // Only remove mobile entities. We don't want to remove walls and doors (that happened).
        if (Entities.AnyOf(c->secondId, bmRobot | bmPlayer | bmShot))
        {
            Entities.Set(c->secondId, bmReap);
        }
    }
}
Events.Clear();
```

This will no doubt evolve with time, but the key takeaway from this is that events are ephemeral. They're added to the *Events* table as they occur, then the event system acts on them before clearing down the table when all events have been processed.

## What if we used SQLite?

I don't think this is necessarily the way forward, but once you start thinking about an ECS in terms of a relational database, it isn't a huge step to using one for prototyping. In other words, if I'd started this project with this realisation, then I could have used an in-memory instance of [SQLite](https://www.sqlite.org/index.html) to prototype the idea, and saved myself from reinventing a few wheels.

[^1]: Yes, it's in the name, but that's rather misleading.