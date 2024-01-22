#ifndef __WINDOW_H__
#define __WINDOW_H__
enum class Window : uint8_t
{
  HALL = 0,
  LAUNDRY,
  BATH_SOUTHWEST,
  BATH_NORTHWEST,  
  JU_WEST,
  JU_NORTH,
  PA_NORTH,
  PA_EAST,
  OFFICE,
  LI_EAST,
  LI_SOUTH,

  Count,
};

const char *windowName[static_cast<uint8_t>(Window::Count)] = 
{
  "Hall",
  "Laundry",
  "Bath_SW",
  "Bath_NW",
  "J_W",
  "J_N",
  "P_N",
  "P_O",
  "office",
  "L_O",
  "L_S"
};

#endif // __WINDOW_H__