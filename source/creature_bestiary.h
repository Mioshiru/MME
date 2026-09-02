#ifndef RME_CREATURE_BESTIARY_H_
#define RME_CREATURE_BESTIARY_H_

#include <string>
#include <vector>
#include <unordered_map>

enum CreatureBestiaryClass {
	BESTIARY_AMPHIBIANS = 0,
	BESTIARY_AQUATIC,
	BESTIARY_BIRDS,
	BESTIARY_CONSTRUCTS,
	BESTIARY_DEMONS,
	BESTIARY_DRAGONS,
	BESTIARY_ELEMENTALS,
	BESTIARY_EXTRA_DIMENSIONAL,
	BESTIARY_FEY,
	BESTIARY_GIANTS,
	BESTIARY_HUMANOIDS,
	BESTIARY_HUMANS,
	BESTIARY_MAGICAL_CREATURES,
	BESTIARY_MAMMALS,
	BESTIARY_PLANTS,
	BESTIARY_REPTILES,
	BESTIARY_UNDEAD,
	BESTIARY_BOSSES,
	BESTIARY_NPCS,
	BESTIARY_OTHERS,
	BESTIARY_CLASS_COUNT
};

enum CreatureDifficultyTier {
	TIER_HARMLESS = 0, // Harmless (25 kills) - Non-aggressive, e.g. Cat, Dog, Deer, Butterfly
	TIER_TRIVIAL,      // Trivial (250 kills) - Extremely weak, e.g. Rat, Bat, Spider, Troll
	TIER_EASY,         // Easy (500 kills) - Mild threat, e.g. Orc, Minotaur, Dwarf, Cyclops
	TIER_MEDIUM,       // Medium (1,000 kills) - Moderate threat, e.g. Hellspawn, Dragon Lord, Werewolf, Hydra
	TIER_HARD,         // Hard (2,500 kills) - Dangerous, e.g. Demon, Grim Reaper, Behemoth, Juggernaut
	TIER_CHALLENGING   // Challenging (5,000 kills) - End-game, e.g. Brachiodemon, Cloak of Terror, Walking Dread
};

struct CreatureBestiaryInfo {
	std::string name;
	std::string bestiary_class;
	CreatureBestiaryClass class_id;
	int health;
	int experience;
	double ratio; // exp / health ratio
	int speed;
	int armor;
	int defense;
	CreatureDifficultyTier tier;
	std::string tier_name;
	std::string elements; // e.g. "Physical, Fire"
	std::string weaknesses; // e.g. "Ice, Holy"
	std::string description;
	std::string wiki_url; // Direct TibiaWiki hyperlink
};

class CreatureBestiary {
public:
	static void Initialize();
	static const CreatureBestiaryInfo* GetInfo(const std::string& name);
	static CreatureBestiaryClass GetClassForCreature(const std::string& name, bool isNpc);
	static std::string GetClassName(CreatureBestiaryClass cid);
	static std::vector<std::string> GetAllClassNames();
	static CreatureDifficultyTier EstimateDifficulty(int hp, int exp);
	static std::string GetTierName(CreatureDifficultyTier tier);
	static std::string GetWikiUrl(const std::string& name);

private:
	static void AddEntry(const std::string& name, CreatureBestiaryClass cid, int hp, int exp, int speed, int armor, int def,
	                     const std::string& elem, const std::string& weak, const std::string& desc);
	static std::unordered_map<std::string, CreatureBestiaryInfo> s_database;
	static bool s_initialized;
};

#endif // RME_CREATURE_BESTIARY_H_
