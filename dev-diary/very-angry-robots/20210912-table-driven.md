# Table Driven Processing in Very Angry Robots III

**12/9/21**

## Multiple Rooms

In terms of the game itself, little has changed. Up until now, there has only been one room in the game. Now the game supports multiple rooms, and the player can move between them. Each room has 4 doorways. The one that the player entered through is always blocked by a locked door. The other doors are open, and the player can exit the room through them. When the player exits a room, a new room is generated, and the view pans from the current room to the new one.

However, to reach that point has involved the introduction of several new systems and tables.

## Table Driven Processing

In a previous post, I referred to implementing an Entity Component System, then I asked "What if we used SQLite?"

In the past week, I seem to have taken that thinking one step further, because the system has evolved to use a pattern that I have seen over and over again in a previous job (in retail) in which there are "processors" that act upon tables in a relational database, and the state of the system is entirely table driven. We certainly didn't call that an ECS in retail, and I've not going to call it that now - it's a common pattern in which systems are coupled via data held in tables. In other words, I've taken to thinking in terms of an [Entity-relationship model](https://en.wikipedia.org/wiki/Entity%E2%80%93relationship_model) and in doing so, have made it much easier for me to think about the system.

So now that I'm free from the constraint of thinking in terms of an ECS, I can reframe my thinking in terms of **Tables** and the **Systems** that act upon them. 

## Systems

The game now has the following systems.

| System             | Usage                                                        |
| ------------------ | ------------------------------------------------------------ |
| CollisionSystem    | Checks for collisions between *Collidable* entities.         |
| DrawingSystem      | Draws *Drawable* entities.                                   |
| EventSystem        | Processes *Events* and dispatches them to event handlers in other systems. |
| GameStatusSystem   | Responsible for the overall game, including room generation and robot management. |
| MovementSystem     | Responsible for moving entities that have a *Position* and a *Velocity*. |
| PlayerActionSystem | Allows the user to control the player via the keyboard or a gamepad. |
| ReaperSystem       | Removes entities that have been marked for removal.          |
| RobotActionSystem  | Runs the logic for each robot. This is currently a placeholder, but eventually the robots will be run by Arviss. |

Systems are run either by the per-frame update, or by the fixed timestep update.

## Systems invoked by the per-frame update

The only system invoked by the per-frame update is the *DrawingSystem*.

## Systems invoked by the fixed timestep update

All other systems are invoked by the fixed timestep update. They run in the following order, by invoking their `Update()` function.

1. ReaperSystem
2. PlayerActionSystem
3. RobotActionSystem
4. MovementSystem
5. CollisionSystem
6. CollisionResponseSystem
7. GameStatusSystem
8. EventSystem

First, the ReaperSystem runs, removing any entities that were marked for removal in the previous timestep. Then the PlayerActionSystem and RobotActionSystem are run to get the desired actions for the player and the robots respectively. The outcome of this is to set the *Velocity* of each entity. The Movement system then applies each entity's *Velocity* to its *Position*. The CollisionSystem checks for collisions between *Collidable* entities and raises a *CollisionEvent* for each collision that it detects. The CollisionResponse system's `Update()` method does nothing, as collision response is event driven. The GameStatus system checks some timers set by its event handler to see if it should take action, then finally the EventSystem runs.

## Event Handling

As a reminder, an event is a record of something that has already happened. The game has the following events.

| Event Type     | Description                                                  |
| -------------- | ------------------------------------------------------------ |
| CollisionEvent | Raised by the CollisionSystem when it detects that a collision has occurred. |
| DoorEvent      | Raised when the player exits a room or enters a room.        |
| PlayerEvent    | Raised when something happens to the player. Currently a PlayerEvent is raised when the player is spawned, or when the player dies. |

When an event is raised, it is appended to the *Events* table.

When the EventSystem's `Update()` method runs, it iterates through the *Events* table, dispatching events to *event handlers* that have been registered with it. This in turn might raise more events, and these in turn are handled, and so on, until there are no events remaining.

In other words, when a fixed timestep starts, the Events table is empty. When systems run, they may raise events. When the EventSystem runs, it consumes *all* events, dispatching them to event handlers.

| Event Handler           | Events Handled | Description                                                  |
| ----------------------- | -------------- | ------------------------------------------------------------ |
| CollisionSystem         | DoorEvent      | Disables the collision system when the player exits a room and enables it when the player enters a room. |
| MovementSystem          | DoorEvent      | Disables the movement system when the player exits a room and enables it when the player enters a room. |
| CollisionResponseSystem | CollisionEvent | Determines which entities have collided with each other and takes appropriate action, including raising further events. For example, it raises a DoorEvent when the player collides with a door trigger. |
| DrawingSystem           | DoorEvent      | Starts a transition between rooms when the player exits a room, and stops it when the player enters a room. |
| GameStatusSystem        | DoorEvent      | Creates a new room when the player exits the current room.   |
| GameStatusSystem        | PlayerEvent    | Responds to the player dying, and the player being spawned.  |
| PlayerActionSystem      | DoorEvent      | Disables the player action system when the player exits a room and enables it when the player enters a room. |
| RobotActionSystem       | DoorEvent      | Disables the robot action system when the player exits a room and enables it when the player enters a room. |

## Tables

Finally, let's talk about Tables. Previously I referred to these as Components, but I don't think that's appropriate terminology any more - let's call them what they are. The entire system is modelled as tables and the systems that act on them, rather than as objects (and conceptually the messages that are passed between them). This might seem quaintly old fashioned, but it's an approach that works, and moreover, it's an approach that lends itself well to multi-threading.

Here are the tables in Very Angry Robots III at the time of writing. Most tables contain data that relates to a game entity, but *Events* does not.

| Table Name     | Key       | Description                                         |
| -------------- | --------- | --------------------------------------------------- |
| Collidables    | entity id | Things that can be collided with.                   |
| Doors          | entity id | Doors in the game.                                  |
| Events         | event id  | Events that occurred in the current time step.      |
| Owners         | entity id | Things owned by a given room.                       |
| PlayerStatuses | entity id | Player status information such as lives and scores. |
| Positions      | entity id | Positions that can be attached to a game entity.    |
| Velocities     | entity id | Velocities that can be attached to a game entity.   |
| Walls          | entity id | Walls in the game.                                  |

Collisions are modelled as a special type of Event, but I'm half tempted to break them out into their own separate table, indexed by collision id.

