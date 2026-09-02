// WAD00_12 -- one function, read from the image with tools/disasm.py:
// LangStringToLangID copies up to 63 characters of the name into a
// buffer it terminates first, takes what follows an underscore as the
// region when the name is longer than two characters, and walks the
// language table comparing the first two characters of the name
// case-insensitively. A name match is remembered; a region match on
// top of it returns that entry's locale id as sixteen bits at once.
// After the walk a remembered name returns its language id, and
// nothing found is 0xFFFF. The table and its size are file statics in
// retail (scope:local), so this unit matches and does not link; the
// entries are twenty bytes and the words read from the image say what
// is where; the locals are declared in the order that gives them
// retail's registers. The underscore literal is the image's own `.data` string
// for this function, which is what a literal handed to a non-const
// `char*` becomes; strstr is declared so here.

extern "C" {
char* strncpy(char* dst, const char* src, unsigned long n);
unsigned long strlen(const char* s);
char* strstr(char* s, char* sub);
int strncasecmp(const char* a, const char* b, unsigned long n);
}

class LanguageName {
public:
    const char* name;
    const char* region;
    const char* fullName;
    unsigned short langId;
    unsigned short subLangId;
    unsigned int lcid;
};

extern LanguageName LanguageNamesArray[];
extern int LanguageArraySize;

unsigned short LangStringToLangID(const char* str) {
    int i;
    const char* region = 0;
    int found;
    char buf[64];

    buf[63] = 0;
    strncpy(buf, str, 63);

    if (strlen(buf) > 2) {
        region = strstr(buf, "_");

        if (region) {
            region = region + 1;
        }
    }

    found = -1;

    for (i = 0; i < LanguageArraySize; i++) {
        if (strncasecmp(str, LanguageNamesArray[i].name, 2) == 0) {
            found = i;

            if (region) {
                if (strncasecmp(region, LanguageNamesArray[i].region, 2) == 0) {
                    return (unsigned short)LanguageNamesArray[i].lcid;
                }
            }
        }
    }

    if (found >= 0) {
        return LanguageNamesArray[found].langId;
    }

    return 0xFFFF;
}
