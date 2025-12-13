* **통신 관련**이나 **시각화 툴 자체** 등등 소스는 여기 들어갑니다
* CAN db파일 자체 혹은 변환하는 헤더파일
* ini 파싱 기능
* 디바이스 드라이버 등
* 저희가 자주 쓰는 define도 여기 들어갑니다 (interface 헤더파일)
```c
// 예시
#ifndef __INTERFACE_CONSTANTS_HPP__
#define __INTERFACE_CONSTANTS_HPP__
#pragma once

// STD Header
#include <stdint.h>
#include <map>
#include <utility>
#include <vector>
#include <cmath>

// 근데 c는 namespace 가 없긴함 흠
namespace interface {
    const double DEG2RAD = M_PI/180.0;
    const double RAD2DEG = 180.0/M_PI;
    const double MPS2KPH = 3600.0/1000.0;
    const double KPH2MPS = 1000.0/3600.0;
    const double EARTH_RADIUS_EQUA = 6378137.0;
```