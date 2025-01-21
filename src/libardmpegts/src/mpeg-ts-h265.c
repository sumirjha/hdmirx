#include "mpeg-types.h"
#include "mpeg-util.h"
#include <assert.h>
#include <string.h>

#define H265_NAL_AUD 35


static int mpeg_h265_is_new_access_unit(const uint8_t* nalu, size_t bytes)
{
    enum { NAL_VPS = 32, NAL_SPS = 33, NAL_PPS = 34, NAL_AUD = 35, NAL_PREFIX_SEI = 39, };
    
    uint8_t nal_type;
    uint8_t nuh_layer_id;
    
    if(bytes < 3)
        return 0;
    
    nal_type = (nalu[0] >> 1) & 0x3f;
    nuh_layer_id = ((nalu[0] & 0x01) << 5) | ((nalu[1] >> 3) &0x1F);
    
    // 7.4.2.4.4 Order of NAL units and coded pictures and their association to access units
    if(NAL_VPS == nal_type || NAL_SPS == nal_type || NAL_PPS == nal_type ||
       (nuh_layer_id == 0 && (NAL_AUD == nal_type || NAL_PREFIX_SEI == nal_type || (41 <= nal_type && nal_type <= 44) || (48 <= nal_type && nal_type <= 55))))
        return 1;
        
    // 7.4.2.4.5 Order of VCL NAL units and association to coded pictures
    if (nal_type <= 31)
    {
        //first_slice_segment_in_pic_flag 0x80
        return (nalu[2] & 0x80) ? 1 : 0;
    }
    
    return 0;
}

int mpeg_h265_find_new_access_unit(const uint8_t* data, size_t bytes, int* vcl)
{
    int n;
    size_t leading;
    uint8_t nal_type;
    const uint8_t* p, *end;

    end = data + bytes;
    for (p = data; p && p < end; p += n)
    {
        n = mpeg_h264_find_nalu(p, end - p, &leading);
        if (n < 0)
            return -1;

        nal_type = (p[n] >> 1) & 0x3f;
        if (*vcl > 0 && mpeg_h265_is_new_access_unit(p+n, end - p - n))
        {
            return (int)(p - data + n - leading);
        }
        else if (nal_type <= 31)
        {
            ++* vcl;
        }
        else
        {
            // nothing to do
        }
    }

    return -1;
}
