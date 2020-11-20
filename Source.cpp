//Fixed
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <functional>

// 1. Это очень важно, ВСЕГДА РАЗДЕЛЯЙТЕ ЛОГИЧЕСКИЕ БЛОКИ ПУСТОЙ СТРОКОЙ.
// Это относится к:
// Разные функциональные объекты, например:
// функция1 и функция2 нужно разделять строкой
// или
// константа и макрос, подключаемые библиотеки , функция, класс, структура или любая пара из.
// или
// логические блоки внутри функции. Это крайне важно, это делает ваш код более лаконичным.
// К примеру для начала вы можете разделять свою функцию на три базовых части
//      1. Подготовка или инициализация
//      2. Действие
//      3. Результат
//  Пример:
//          bool myFoo(int g, string e){
//                  if(e.empty) return;
//                  if(g < 1) return;
//                  const auto index = GetGlobalIndex();
//
//                  auto result = e.size() > g ? g : 0;
//                  result = (result > 0) ? g + index : 0;
//
//                  return result > 0;
//          }
//
// 2. Посмотрите все замечания которые я вам оставил и ознакомьтесь с материалами. Помните, что это нужно в первую очередь вам и не ленитесь
//
// Результат зачет, но можно лучше.

using namespace std;

//1. Используйте constexpr там где это возможно
//"Эффективный и современный C++" Скотт Майерс
//2. Используйте путсую линию чтобы отделять логические блоки.
//Нужно приложить усилия чтобы понять где функция а где константа
const int MAX_RESULT_DOCUMENT_COUNT = 5;
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    //Используйте константную ссылку вместо копирования элементов коллекции в цикле
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            //Используйте функцию clear() для очистки строки
			//https://stackoverflow.com/questions/35388912/why-is-there-clear-method-when-can-be-assigned-to-stdstring
            word = "";
        }
        else {
            word += c;
        }
    }
    words.push_back(word);
    return words;
}
struct Document {
    int id;
    double relevance;
    int rating;
};
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    //Обязательно делайте зазоры между функциями в одну пустую строку. Иначе код нечитаем.
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsWithoutStop(document);
        if (words.empty()){ return; }
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }
    //Обязательно делайте зазоры между функциями в одну пустую строку. Иначе код нечитаем.
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        auto fun_pred = [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; };
        return FindTopDocuments(raw_query, fun_pred);
    }

    vector<Document> FindTopDocuments(const string& raw_query, function<bool(int, DocumentStatus, int)> fun_pred) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, fun_pred);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
				// 1. Используйте модификатор const всегда когда это возможно
                // 2. В данном случае, можно не использовать переменную на стеки а сразу ее вернуть
                //    return (a > b) ? a : b;
                bool res = abs(lhs.relevance - rhs.relevance) < 1e-6 ? lhs.rating > rhs.rating :  lhs.relevance > rhs.relevance;
                return res;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    size_t GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            //У вас в двух циклах одно и тоже условие.
            //Можно сделать одну функцию принимающую word и возвращающую bool 
            //или использовать предикат в виде лямбды, который будет создаваться в начале функции
            if (word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    vector<string> SplitIntoWordsWithoutStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    bool IsStopWord(const string& word) const {
        // Вы используете лишнюю операцию сравнения
		// http://www.cplusplus.com/reference/set/set/count/
		// http://eelis.net/c++draft/conv.prom#6
        return stop_words_.count(word) > 0;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            //Можно инвертировать цикл и убрать двойную вложеность
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    vector<Document> FindAllDocuments(const Query& query, function<bool(int, DocumentStatus, int)> fun_pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            //Название переменной очень странное. В данном случае вы можете использовать однобуквенное название
            // например w, так как область видимости переменной ограничивается небольшим циклом
            for (const auto& wtdf : word_to_document_freqs_.at(word)) {
                if (fun_pred(wtdf.first, documents_.at(wtdf.first).status, documents_.at(wtdf.first).rating)) {
                    document_to_relevance[wtdf.first] += wtdf.second * inverse_document_freq;
                }
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            //Название переменной очень странное. В данном случае вы можете использовать однобуквенное название
			// например w, так как область видимости переменной ограничивается небольшим циклом
            for (const auto& wtdf : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(wtdf.first);
            }
        }
        vector<Document> matched_documents;
        for (const auto& dtr : document_to_relevance) {
            //Код плохо читается, лучше сделать так как я приведу ниже и всегда придерживаться подобного стиля
            //            matched_documents.push_back({
            //                                          dtr.first,
            //                                          dtr.second,
            //                                          documents_.at(dtr.first).rating
			//	                                      }
        	//              );
            // Так более понятно где аргументы и где конец выражения. Обратите внимание, что закрывающая скобка  стоит на отдельной строке
            //на уровне начала выражения. Это стандартная практика. Я вам советую придерживаться такого стиля.
            matched_documents.push_back({
                dtr.first,
                dtr.second,
                documents_.at(dtr.first).rating
                });
        }
        return matched_documents;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int& rating : ratings) {
            rating_sum += rating;
        }
        //Используйте метод empty, если нужно проверить пустой ли контейнер или нет
        //https://www.cplusplus.com/reference/vector/vector/empty/
        return ratings.size() == 0 ? 0 : rating_sum / static_cast<int>(ratings.size());
    }
};
// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;

    const auto predicate = [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; };
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, predicate)) {
        PrintDocument(document);
    }
    return 0;
}