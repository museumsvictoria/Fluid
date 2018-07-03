//
//  RotaryEncoders.cxx
//  RotaryTest
//
//  Created by Andrew Wright on 24/4/18.
//

#include "RotaryEncoders.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"
#include "CinderImGui.h"

using namespace ci;

static const float kMaxValue = 4096.0f;

RotaryEncoders::RotaryEncoders()
{
    try
    {
#ifdef CINDER_COCOA
        _serial = Serial::create(Serial::findDeviceByNameContains("wchusbserial1"), 9600);
#else
        _serial = Serial::create(Serial::findDeviceByNameContains("COM7"), 9600);
#endif
        _updateConnection = app::App::get()->getSignalUpdate().connect([=] { Tick(); });
    }
    catch (const std::exception& e)
    {

    }
    
    for ( int i = 0; i < NumValues(); i++ )
    {
        _values[i] = 0.0f;
        _zeroes[i] = 0.0f;
    }
    
    Load ( );
}

void RotaryEncoders::Tick()
{
    static std::stringstream kRunningBuffer;

    if (_serial->getNumBytesAvailable() > 0)
    {
        auto numBytes = _serial->getNumBytesAvailable();

        std::string bytes;
        bytes.resize(numBytes);

        _serial->readBytes((char *)bytes.data(), bytes.size());

        auto lines = ci::split(bytes, "\r\n");

        for (int i = 0; i < lines.size() - 1; i++)
        {
            auto& line = lines[i];
            if (!line.empty())
            {
                if (line[0] != ',' && line.find(",") != std::string::npos)
                {
                    auto lineParts = ci::split(line, ",");
                    int index = -1;
                    int value = -1;

                    try
                    {
                        index = boost::lexical_cast<int>(lineParts[0]);
                        value = boost::lexical_cast<int>(lineParts[1]);
                    }
                    catch (const std::exception& e)
                    {

                    }

                    if (index >= 0 && index < _values.size())
                    {
                        if (value >= 0 && value < kMaxValue)
                        {
                            _values[index] = static_cast<float>(value);
                        }
                    }
                }
            }
        }

        _serial->flush();
    }
}

void RotaryEncoders::Inspect ( )
{
    if ( ui::CollapsingHeader ( "Rotary Encoders" ) )
    {
        ui::Columns(4, nullptr, false);
        ui::Text( "Name" ); ui::NextColumn();
        ui::Text( "Raw" ); ui::NextColumn();
        ui::Text( "Zero" ); ui::NextColumn();
        ui::Text( "Corrected" ); ui::NextColumn();
        
        for ( int i = 0; i < NumValues(); i++ )
        {
            ui::ScopedId id { i };
            
            float rawValue = _values[i];
            float corrected = ValueAt(i);
            
            ui::Text( "%s", ( "Encoder[" + std::to_string(i) + "]" ).c_str() ); ui::NextColumn();
            ui::Text( "%03.2f", rawValue ); ui::NextColumn();
            ui::DragFloat( "##Z", &_zeroes[i], 0.5f, 0.0f, kMaxValue, "%03.2f" ); ui::NextColumn();
            ui::Text( "%03.2f", corrected * kMaxValue ); ui::NextColumn();
        }
        
        ui::Columns(1);
        if ( ui::Button( "Save" ) ) Save(); ui::SameLine();
        if ( ui::Button( "Load" ) ) Load(); ui::SameLine();
        if ( ui::Button( "Zero" ) )
        {
            _zeroes = _values;
        }
    }
}

void RotaryEncoders::Load ( )
{
    try
    {
        JsonTree tree { app::loadAsset( "EncoderZeroes.json" ) };
        int c = 0;
        for ( auto& v : tree )
        {
            _zeroes[c++] = v.getValue<float>();
        }
    }catch ( const std::exception& e )
    {
        
    }
}

void RotaryEncoders::Save ( )
{
    JsonTree tree = JsonTree::makeArray("V");
    for ( int i = 0; i < _zeroes.size(); i++ )
    {
        tree.pushBack( JsonTree ( "", _zeroes[i] ) );
    }
    
    try
    {
        tree.write ( app::getAssetDirectories().front() / "EncoderZeroes.json" );
    }catch ( const std::exception& e )
    {
        
    }
}

float RotaryEncoders::ValueAt(int index) const
{
    float rawValue = _values[index];
    float zeroedValue = rawValue - _zeroes[index];
    
    if ( zeroedValue < 0.0f      ) zeroedValue += kMaxValue;
    if ( zeroedValue > kMaxValue ) zeroedValue -= kMaxValue;
    
    return zeroedValue / kMaxValue;
}

RotaryEncoders::~RotaryEncoders()
{
    _updateConnection.disconnect();
    if (_serial)
    {

    }
    _serial = nullptr;
}

