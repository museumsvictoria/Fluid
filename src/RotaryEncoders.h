//
//  RotaryEncoders.h
//  RotaryTest
//
//  Created by Andrew Wright on 24/4/18.
//

#ifndef RotaryEncoders_h
#define RotaryEncoders_h

#include "cinder/Serial.h"
#include "cinder/Signals.h"
#include <array>

using RotaryEncodersRef = std::unique_ptr<class RotaryEncoders>;
class RotaryEncoders
{
public:

	RotaryEncoders          ( );
	~RotaryEncoders         ( );

	int                     NumValues   (  ) const { return static_cast<int>(_values.size()); };
	float                   ValueAt     (int index) const;

    bool                    IsConnected ( ) const { return _serial != nullptr; }
    void                    Inspect     ( );

protected:

	void                    Tick        ( );
    void                    Save        ( );
    void                    Load        ( );

    std::array<float, 6>    _zeroes;
	std::array<float, 6>    _values;
	ci::SerialRef           _serial{ nullptr };
	ci::signals::Connection _updateConnection;
};

#endif /* RotaryEncoders_h */
