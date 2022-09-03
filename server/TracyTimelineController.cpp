#include "imgui.h"

#include "TracyTimelineController.hpp"

namespace tracy
{

TimelineController::TimelineController( View& view, Worker& worker )
    : m_height( 0 )
    , m_offset( 0 )
    , m_scroll( 0 )
    , m_firstFrame( true )
    , m_view( view )
    , m_worker( worker )
{
}

void TimelineController::FirstFrameExpired()
{
    m_firstFrame = false;
}

void TimelineController::Begin()
{
    m_items.clear();
}

void TimelineController::End( double pxns, int offset, const ImVec2& wpos, bool hover, float yMin, float yMax )
{
    for( auto& item : m_items )
    {
        item->Draw( m_firstFrame, pxns, offset, wpos, hover, yMin, yMax );
    }

    const auto scrollPos = ImGui::GetScrollY();
    if( scrollPos == 0 && m_scroll != 0 )
    {
        m_height = 0;
    }
    else
    {
        if( offset > m_height ) m_height = offset;
    }
    m_scroll = scrollPos;
}

float TimelineController::AdjustThreadPosition( VisData& vis, float wy, int& offset )
{
    if( vis.offset < offset )
    {
        vis.offset = offset;
    }
    else if( vis.offset > offset )
    {
        const auto diff = vis.offset - offset;
        const auto move = std::max( 2.0, diff * 10.0 * ImGui::GetIO().DeltaTime );
        offset = vis.offset = int( std::max<double>( vis.offset - move, offset ) );
    }

    return offset + wy;
}

void TimelineController::AdjustThreadHeight( VisData& vis, int oldOffset, int& offset )
{
    const auto h = offset - oldOffset;
    if( vis.height > h )
    {
        vis.height = h;
        offset = oldOffset + vis.height;
    }
    else if( vis.height < h )
    {
        if( m_firstFrame )
        {
            vis.height = h;
            offset = oldOffset + h;
        }
        else
        {
            const auto diff = h - vis.height;
            const auto move = std::max( 2.0, diff * 10.0 * ImGui::GetIO().DeltaTime );
            vis.height = int( std::min<double>( vis.height + move, h ) );
            offset = oldOffset + vis.height;
        }
    }
}

}
