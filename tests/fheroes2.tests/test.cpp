#include "pch.h"
#define CAN_OVERRIDE_RENDERING 1
#include "agg.h"
#include "battle_only.h"
#include "image_tool.h"
#include "heroes_indicator.h"
#include "heroes.h"
#include "race.h"
#include "screen.h"
#include "skill.h"
#include "settings.h"
#include "translations.h"
#include <battle_arena.h>
#include <world.h>

class FakeCursor : public fheroes2::Cursor
{
public:
    FakeCursor()
    {
        _emulation = false;
    }
};

class FakeDisplay : public fheroes2::BaseRenderEngine
{
public:
    fheroes2::Sprite sprite;
    FakeDisplay() {
        sprite._disableTransformLayer();
    }
    
    virtual fheroes2::Size getCurrentScreenResolution() const override
    {
        return fheroes2::Size( sprite.width(), sprite.height() );
    }

protected:
    bool allocate( int32_t & width, int32_t & height, bool isFullScreen ) override
    {
        std::cout << "allocate" << std::endl;
        sprite.resize( width, height );
        linkRenderSurface( sprite.image() );
        return true;
    }
};

TEST(SettingsLoading, SimpleSettingsTest) {
    Settings & conf = Settings::Get();

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

class BattleModifierSkills : public ::testing::TestWithParam<int>
{
public:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
        AGG::Init();
        Translation::setStripContext( '|' );
    }

protected:
    virtual void SetUp() override
    {
        std::cout << "SetUp"  << std::endl;
        fheroes2::Display & display = fheroes2::Display::instance();
        engine = new FakeDisplay();
        display.setEngine( std::unique_ptr<fheroes2::BaseRenderEngine>( engine ) );
        display.setCursor( std::unique_ptr<fheroes2::Cursor>( new FakeCursor() ) );
    }

    virtual void TearDown() override
    {
        std::cout << "TearDown"  << std::endl;
        fheroes2::Display & display = fheroes2::Display::instance();
        display.resize( 1, 1 );
    }

protected:
    FakeDisplay * engine;
};

TEST_P( BattleModifierSkills, DrawLuckModifier )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    display.resize( 100, 100 );
    EXPECT_FALSE( engine->sprite.empty() );
    const int skill = GetParam();
    std::cout << "DLM" << skill << std::endl;

    Heroes hero( Heroes::LORDKILBURN, Race::KNGT );
    if ( !hero.HasSecondarySkill( Skill::Secondary::LUCK ) ) {
        hero.LearnSkill( Skill::Secondary( Skill::Secondary::LUCK, skill ) );
    }

    LuckIndicator luck( &hero );
    luck.Redraw();
    std::cout << "REDRAW" << skill << std::endl;

    std::string str_level[] = { "None", "Basic", "Advanced", "Expert" };
    std::string dstfile = "luck_" + str_level[skill];

    std::cout << dstfile << std::endl;
#ifndef WITH_IMAGE
    dstfile += ".bmp";
#else
    dstfile += ".png";
#endif

    EXPECT_FALSE( engine->sprite.empty() );
    fheroes2::Save( engine->sprite, dstfile, 0 );

    SUCCEED();
}

TEST_P( BattleModifierSkills, DrawMoraleModifier )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    display.resize( 200, 200 );
    const int skill = GetParam();

    Heroes hero( Heroes::LORDKILBURN, Race::KNGT );
    if ( !hero.HasSecondarySkill( Skill::Secondary::LEADERSHIP ) ) {
        hero.LearnSkill( Skill::Secondary( Skill::Secondary::LEADERSHIP, skill ) );
    }

    MoraleIndicator indicator( &hero );
    indicator.Redraw();

    std::string str_level[] = { "None", "Basic", "Advanced", "Expert" };
    std::string dstfile = "morale_" + str_level[skill];

#ifndef WITH_IMAGE
    dstfile += ".bmp";
#else
    dstfile += ".png";
#endif

    EXPECT_FALSE( engine->sprite.empty() );
    fheroes2::Save( engine->sprite, dstfile, 0 );

    SUCCEED();
}

INSTANTIATE_TEST_CASE_P( 
    BattleModifierSkillsInstantiation,
    BattleModifierSkills, ::testing::Values( Skill::Level::NONE, Skill::Level::BASIC, Skill::Level::ADVANCED, Skill::Level::EXPERT ) );



TEST( BattleOnly, BattleOnlyTest )
{
    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( Color::RED | Color::BLUE );

    world.NewMaps( 10, 10 );

    // This will trigger autobattle when no interface.
    Players::SetPlayerControl( Color::RED, CONTROL_HUMAN );
    Players::SetPlayerControl( Color::BLUE, CONTROL_HUMAN );

    // Select heroes and armies
    Heroes hero1( Heroes::LORDKILBURN, Race::KNGT );
    hero1.SetColor( Color::RED );
    Army army1( &hero1 );
    army1.GetTroop( 0 )->Set( Monster::PEASANT, 100 );

    Heroes hero2( Heroes::SIRGALLANTH, Race::KNGT );
    hero2.SetColor( Color::BLUE );
    Army army2( &hero2 );
    army2.GetTroop( 0 )->Set( Monster::PEASANT, 1 );
    
    Battle::Arena arena( army1, army2, 0, false );

    while ( arena.BattleValid() )
        arena.Turns();

    auto & results = arena.GetResult();

    EXPECT_TRUE( results.AttackerWins() );
}

TEST( Information, NecromancySkillForUnknownHeros )
{
    Settings & conf = Settings::Get();

    conf.GetPlayers().Init( Color::RED | Color::BLUE );

    world.NewMaps( 10, 10 );

    // This will trigger autobattle when no interface.
    Players::SetPlayerControl( Color::RED, CONTROL_HUMAN );
    Players::SetPlayerControl( Color::BLUE, CONTROL_HUMAN );

    // Select heroes and armies
    Heroes hero( Heroes::LORDKILBURN, Race::KNGT );
    hero.SetColor( Color::RED );

    Skill::Secondary skill( Skill::Secondary::NECROMANCY, 1 );

    EXPECT_EQ( std::string("Basic Necromancy allows 10 percent of the creatures killed in combat to be brought back from the dead as Skeletons."), skill.GetDescription( hero ) );
}