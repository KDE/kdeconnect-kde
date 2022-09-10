/**
 * SPDX-FileCopyrightText: 2020 Jiří Wolker <woljiri@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

/**
 * Map used to determine that ASCII character is in GSM 03.38 7-bit alphabet.
 *
 * Only allowed control characters are CR and LF but GSM alphabet has more of them.
 */
bool gsm_ascii_map[] = {
    false, // 0x0, some control char
    false, // 0x1, some control char
    false, // 0x2, some control char
    false, // 0x3, some control char
    false, // 0x4, some control char
    false, // 0x5, some control char
    false, // 0x6, some control char
    false, // 0x7, some control char
    false, // 0x8, some control char
    false, // 0x9, some control char
    true, // 0xA, LF
    false, // 0xB, some control char
    false, // 0xC, some control char
    true, // 0xD, CR
    false, // 0xE, some control char
    false, // 0xF, some control char
    false, // 0x10, some control char
    false, // 0x11, some control char
    false, // 0x12, some control char
    false, // 0x13, some control char
    false, // 0x14, some control char
    false, // 0x15, some control char
    false, // 0x16, some control char
    false, // 0x17, some control char
    false, // 0x18, some control char
    false, // 0x19, some control char
    false, // 0x1A, some control char
    false, // 0x1B, some control char
    false, // 0x1C, some control char
    false, // 0x1D, some control char
    false, // 0x1E, some control char
    false, // 0x1F, some control char
    true, // 20, space
    true, // 21, !
    true, // 22, "
    true, // 23, #
    true, // 24, $
    true, // 25, %
    true, // 26, &
    true, // 27, '
    true, // 28, (
    true, // 29, )
    true, // 2A, *
    true, // 2B, +
    true, // 2C, ,
    true, // 2D, -
    true, // 2E, .
    true, // 2F, /
    true, // 30, 0
    true, // 31, 1
    true, // 32, 2
    true, // 33, 3
    true, // 34, 4
    true, // 35, 5
    true, // 36, 6
    true, // 37, 7
    true, // 38, 8
    true, // 39, 9
    true, // 3A, :
    true, // 3B, ;
    true, // 3C, <
    true, // 3D, =
    true, // 3E, >
    true, // 3F, ?
    true, // 40, @
    true, // 41, A
    true, // 42, B
    true, // 43, C
    true, // 44, D
    true, // 45, E
    true, // 46, F
    true, // 47, G
    true, // 48, H
    true, // 49, I
    true, // 4A, J
    true, // 4B, K
    true, // 4C, L
    true, // 4D, M
    true, // 4E, N
    true, // 4F, O
    true, // 50, P
    true, // 51, Q
    true, // 52, R
    true, // 53, S
    true, // 54, T
    true, // 55, U
    true, // 56, V
    true, // 57, W
    true, // 58, X
    true, // 59, Y
    true, // 5A, Z
    false, // 5B, [
    false, // 5C, backslash
    false, // 5D, ]
    false, // 5E, ^
    true, // 5F, _
    false, // 60, `
    true, // 61, a
    true, // 62, b
    true, // 63, c
    true, // 64, d
    true, // 65, e
    true, // 66, f
    true, // 67, g
    true, // 68, h
    true, // 69, i
    true, // 6A, j
    true, // 6B, k
    true, // 6C, l
    true, // 6D, m
    true, // 6E, n
    true, // 6F, o
    true, // 70, p
    true, // 71, q
    true, // 72, r
    true, // 73, s
    true, // 74, t
    true, // 75, u
    true, // 76, v
    true, // 77, w
    true, // 78, x
    true, // 79, y
    true, // 7A, z
    false, // 7B, {
    false, // 7C, |
    false, // 7D, }
    false, // 7E, ~
};
