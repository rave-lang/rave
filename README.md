# Rave

**Note - All of this is more of a concept stage and is not yet implemented**

Rave is a isolated programming environment for creating multi-user applications.
The language compiles to native binaries with an Entity/Component based RPC structure with an interpreter (or optional jit) to execute dynamic code.

The main structure 'Streams' combines data and logic into a single structure which it describes a buffer of data. its relations, spans, offsets and more and logical branching.
- Regular structs are simply a static stream. 
- Member functions can be seen as tagged unions of a stream, these can also be events if empty that can be mountable.
- Stacking streams and is the way you handle messages and events, as well as how you can define concurrency of a whole program.

The main reason for this structure is to provide memory safety in a dynamic environment, as well as automatic persistance and data serialization with lazy-loaded data between users.
This is primarily in the dynamic environment which doesnt rely on the statically compiled code.
The native part of the code is allowed to do unsafe operations but needs to explicitly mark lifetimes and spans of c api functions.
