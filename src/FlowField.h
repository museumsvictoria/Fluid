//
//  FlowField.h
//  Fluid
//
//  Created by Andrew Wright on 25/3/18.
//

#ifndef FlowField_h
#define FlowField_h

#include "Fluid.h"
#include <Time/Property.h>

using FlowFieldRef = std::unique_ptr<class FlowField>;
class FlowField
{
public:
    FlowField           ( Fluid::Sim * fluid );
    
    void                Draw    ( );
    void                Inspect ( );

    Time::FloatProperty Alpha{0.0f};
    Time::FloatProperty ColorWeight{0.3f};
    Time::FloatProperty Weight{0.4f};
    
protected:
    
    Fluid::Sim *        _fluid;
};

#endif /* FlowField_h */
