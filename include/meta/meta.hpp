#pragma once

enum class GameFamily
{
    Blupi,
    Eggbert,
    ReaktivniyEgbert
};

enum class GameGeneration
{
    V1,
    V2
};

enum class Distribution
{
    Full,
    Demo,
    Valueware
};

enum class Language
{
    EN,
    FR,
    RU
};

struct GameVariant
{
    GameFamily family;
    GameGeneration generation;
    Distribution distribution;
    Language language;
};