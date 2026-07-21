/*
 * Copyright (c) 2020-2026 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stratosphere.hpp>

namespace ams::utils {

    template <typename T>
    struct Vec3d {
        T x;
        T y;
        T z;
    } PACKED;

    template<std::floating_point T>
    class Quaternion {
        public:
            union {
                T raw[4];
                struct {
                    T x;
                    T y;
                    T z;
                    T w;
                };
            };

        public:
            constexpr Quaternion() : x(0), y(0), z(0), w(1) { };
            constexpr Quaternion(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { };

        public:
            constexpr Quaternion operator+() const {
                return *this;
            }

            constexpr Quaternion operator-() const {
                return Quaternion(-x, -y, -z, -w);
            }

            constexpr Quaternion operator+(Quaternion rhs) const {
                return Quaternion(x + rhs.x,
                                y + rhs.y,
                                z + rhs.z,
                                w + rhs.w);
            }

            constexpr Quaternion operator-(Quaternion rhs) const {
                return Quaternion(x - rhs.x,
                                y - rhs.y,
                                z - rhs.z,
                                w - rhs.w);
            }

            constexpr Quaternion operator*(Quaternion rhs) const {
                return Quaternion(
                    w*rhs.x + x*rhs.w + y*rhs.z - z*rhs.y,
                    w*rhs.y + y*rhs.w + z*rhs.x - x*rhs.z,
                    w*rhs.z + z*rhs.w + x*rhs.y - y*rhs.x,
                    w*rhs.w - x*rhs.x - y*rhs.y - z*rhs.z
                );
            }

            constexpr Quaternion operator*(T s) const {
                return Quaternion(x*s, y*s, z*s, w*s);
            }

            constexpr Quaternion operator/(T s) const {
                T inv = T{1} / s;
                return Quaternion(x*inv, y*inv, z*inv, w*inv);
            }

            constexpr Quaternion& operator+=(Quaternion rhs) {
                x += rhs.x;
                y += rhs.y;
                z += rhs.z;
                w += rhs.w;
                return *this;
            }

            constexpr Quaternion& operator-=(Quaternion rhs) {
                x -= rhs.x;
                y -= rhs.y;
                z -= rhs.z;
                w -= rhs.w;
                return *this;
            }

            constexpr Quaternion& operator*=(T s) {
                x *= s;
                y *= s;
                z *= s;
                w *= s;
                return *this;
            }

            constexpr Quaternion& operator/=(T s) {
                T inv = T{1} / s;
                return *this *= inv;
            }

        public:
            static constexpr Quaternion Identity() {
                return Quaternion(0, 0, 0, 1);
            }
            
            static constexpr Quaternion Add(Quaternion q1, Quaternion q2) {
                return Quaternion(q1.x+q2.x, q1.y+q2.y, q1.z+q2.z, q1.w+q2.w);
            }

            static constexpr Quaternion Subtract(Quaternion q1, Quaternion q2) {
                return Quaternion(q1.x-q2.x, q1.y-q2.y, q1.z-q2.z, q1.w-q2.w);
            }

            static constexpr Quaternion Multiply(Quaternion q1, Quaternion q2) {
                return Quaternion(q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y,
                                  q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z,
                                  q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x,
                                  q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z);
            }

            static constexpr Quaternion Multiply(Quaternion q, T s) {
                return Quaternion(q.x*s, q.y*s, q.z*s, q.w*s);
            }

            static constexpr Quaternion Divide(Quaternion q, T s) {
                T s_inv = T{1} / s;
                return Quaternion(q.x*s_inv, q.y*s_inv, q.z*s_inv, q.w*s_inv);
            }
            
            static constexpr T LengthSquared(Quaternion q) {
                return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
            }

            static constexpr T Length(Quaternion q) {
                return std::sqrt(LengthSquared(q));
            }

            static constexpr Quaternion Inverse(Quaternion q) {
                T norm_squared_inv = T{1} / LengthSquared(q);
                return Quaternion(-q.x * norm_squared_inv,
                                  -q.y * norm_squared_inv,
                                  -q.z * norm_squared_inv,
                                   q.w * norm_squared_inv);
            }

            static constexpr Quaternion Conjugate(Quaternion q) {
                return Quaternion(-q.x, -q.y, -q.z, q.w);
            }

            static constexpr Quaternion Normalize(Quaternion q) {
                T norm_inv = T{1} / Length(q);
                return Quaternion(q.x * norm_inv,
                                    q.y * norm_inv,
                                    q.z * norm_inv,
                                    q.w * norm_inv);
            }

            static constexpr T Dot(Quaternion q1, Quaternion q2) {
                return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
            }

            static constexpr Vec3d<T> ToEuler(Quaternion q) {
                T sinr_cosp = T{2} * (q.w*q.x + q.y*q.z);
                T cosr_cosp = T{1} - T{2} * (q.x*q.x + q.y*q.y);
                T roll = std::atan2(sinr_cosp, cosr_cosp);

                T sinp = T{2} * (q.w*q.y - q.z*q.x);
                T pitch = (std::fabs(sinp) >= T{1}) ? std::copysign(std::numbers::pi_v<T>/2, sinp) : std::asin(sinp);

                T siny_cosp = T{2} * (q.w*q.z + q.x*q.y);
                T cosy_cosp = T{1} - T{2} * (q.y*q.y + q.z*q.z);
                T yaw = std::atan2(siny_cosp, cosy_cosp);

                return Vec3d<T> {
                    .x = roll,
                    .y = pitch,
                    .z = yaw
                };
            }
    };

}
