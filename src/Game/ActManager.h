#ifndef FC_GAME_ACT_MANAGER_H
#define FC_GAME_ACT_MANAGER_H

#include "../Graphics/Environment.h"
#include "../Data/Accessor.h"

#include <chrono>
#include <vector>

#include "ACT/BaseTurret.h"
#include "ACT/ItemPickup.h"
#include "ACT/NeutralTurret.h"
#include "ACT/Prop.h"
#include "ACT/SkyCaptain.h"

namespace Game {

class ActManager {
public:
    template <class ActorClass>
    struct SpawnableActor {
        struct Spawner {
            Data::Mission::ACTResource::tSAC_chunk timings;
            ActorClass actor;
            std::chrono::microseconds time;
            std::vector<ActorClass> current_actors;
        };
        std::vector<ActorClass> actors;
        std::vector<Spawner> spawners;
    };

private:
    SpawnableActor<ACT::BaseTurret>    base_turrets;
    SpawnableActor<ACT::ItemPickup>    item_pickups;
    SpawnableActor<ACT::NeutralTurret> neutral_turrets;
    SpawnableActor<ACT::Prop>          props;
    SpawnableActor<ACT::SkyCaptain>    sky_captains;

public:
    ActManager( const Data::Mission::IFF& resource, const Data::Accessor& accessor );
    virtual ~ActManager();

    void initialize( MainProgram &main_program );

    void update( MainProgram &main_program, std::chrono::microseconds delta );
};

}

#endif // FC_MENU_H
