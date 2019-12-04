auto EXPECT_LINE = [] (std::istream& log, std::string expect)
{
    auto tolower = [] (std::string s)
    {
        for (auto& c : s)
            c = std::tolower(c);
        return s;
    };

    bool found = false;

    std::string line;
    while (!found && std::getline(log, line))
    {
        /* Case-insensitive matching */
        line = tolower(line);
        expect = tolower(expect);
        found |= (line.find(expect) != std::string::npos);
    }

    CHECK(found);
};
