const app = document.getElementById("app");

const API_URL = "https://potterapi-fedeperin.vercel.app/en/houses";

const housesUI = [
    { slug: "gryffindor", name: "Gryffindor", image: "assets/gryffindor.jpg" },
    { slug: "slytherin", name: "Slytherin", image: "assets/slytherin.png" },
    { slug: "ravenclaw", name: "Ravenclaw", image: "assets/ravenclaw.jpg" },
    { slug: "hufflepuff", name: "Hufflepuff", image: "assets/hufflepuff.jpg" }
];

let cachedHouses = null;

// Fetch houses from API:
async function fetchHouses() {
    if (cachedHouses) return cachedHouses;

    try {
        const response = await fetch(API_URL);
        if (!response.ok) throw new Error("API error");
        const data = await response.json();
        cachedHouses = data;
        return data;
    } catch (error) {
        app.innerHTML = "<p>Unable to load data. Please try again later.</p>";
        throw error;
    }
}

// Home page:
function renderHome() {
    app.innerHTML = "";

    housesUI.forEach(house => {
        const card = document.createElement("div");
        card.className = "house-card";

        card.innerHTML = `
            <img src="${house.image}" alt="${house.name}">
            <h2>${house.name}</h2>
        `;

        card.addEventListener("click", () => {
            window.location.hash = `/house/${house.slug}`;
        });

        app.appendChild(card);
    });
}

// House detail page: 
async function renderHouse(slug) {
    app.innerHTML = "<p>Loading...</p>";

    const houses = await fetchHouses();
    const house = houses.find(h =>
        h.house.toLowerCase() === slug
    );

    if (!house) {
        app.innerHTML = "<p>House not found.</p>";
        return;
    }

    app.innerHTML = `
        <div class="house-details">
            <h2>${house.house}</h2>
            <p><strong>Founder:</strong> ${house.founder}</p>
            <p><strong>Colors:</strong> ${house.colors.join(", ")}</p>
            <p><strong>Animal:</strong> ${house.animal}</p>
            <button id="back">Back</button>
        </div>
    `;

    document.getElementById("back").onclick = () => {
        window.location.hash = "";
    };
}

// Router:
function router() {
    const hash = window.location.hash;

    if (!hash || hash === "#") {
        renderHome();
        return;
    }

    const parts = hash.replace("#/", "").split("/");

    if (parts[0] === "house" && parts[1]) {
        renderHouse(parts[1]);
    } else {
        app.innerHTML = "<p>Page not found</p>";
    }
}

window.addEventListener("hashchange", router);
router();

